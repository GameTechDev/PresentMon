#include "../../CommonUtilities/win/WinAPI.h"
#include "Interprocess.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionPopulators.h"
#include "SharedMemoryTypes.h"
#include "OwnedDataSegment.h"
#include "ViewedDataSegment.h"
#include "IntrospectionCloneAllocators.h"
#include "../../CommonUtilities/win/Security.h"
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <chrono>
#include "../../PresentMonService/GlobalIdentifiers.h"
#include <windows.h>
#include <sddl.h>
#include <optional>
#include <unordered_map>

namespace pmon::ipc
{
    namespace bip = boost::interprocess;
    namespace vi = std::views;
    namespace rn = std::ranges;

    namespace
    {
        class CommsBase_
        {
        protected:
            static constexpr size_t frameRingSize_ = 5'000;
            static constexpr size_t telemetryRingSize_ = 5'000;
            static constexpr size_t introShmSize_ = 0x10'0000;
            static constexpr const char* introspectionRootName_ = "in-root";
            static constexpr const char* introspectionMutexName_ = "in-mtx";
            static constexpr const char* introspectionSemaphoreName_ = "in-sem";
        };

        class ServiceComms_ : public ServiceComms, CommsBase_
        {
        public:
            ServiceComms_(std::string prefix)
                :
                namer_{ std::move(prefix) },
                shm_{ bip::create_only, namer_.MakeIntrospectionName().c_str(),
                    introShmSize_, nullptr, Permissions_{} },
                pIntroMutex_{ ShmMakeNamedUnique<bip::interprocess_sharable_mutex>(
                    introspectionMutexName_, shm_.get_segment_manager()) },
                pIntroSemaphore_{ ShmMakeNamedUnique<bip::interprocess_semaphore>(
                    introspectionSemaphoreName_, shm_.get_segment_manager(), 0) },
                pRoot_{ ShmMakeNamedUnique<intro::IntrospectionRoot>(introspectionRootName_,
                    shm_.get_segment_manager(), shm_.get_segment_manager()) },
                systemShm_{ namer_.MakeSystemName(),
                    static_cast<const bip::permissions&>(Permissions_{}) }
            {
                PreInitializeIntrospection_();
            }
            intro::IntrospectionRoot& GetIntrospectionRoot() override
            {
                return *pRoot_;
            }
            void RegisterGpuDevice(PM_DEVICE_VENDOR vendor,
                std::string deviceName,
                const MetricCapabilities& caps) override
            {
                auto lck = LockIntrospectionMutexExclusive_();
                const auto deviceId = nextDeviceIndex_++;
                intro::PopulateGpuDevice(
                    shm_.get_segment_manager(), *pRoot_,
                    deviceId, vendor, deviceName, caps
                );
                // allocate map node and create shm segment
                auto& gpuShm = gpuShms_.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(deviceId),
                    std::forward_as_tuple(namer_.MakeGpuName(deviceId))
                ).first->second;
                // populate rings based on caps
                for (auto&& [m, count] : caps) {
                    const auto& metric = pRoot_->FindMetric(m);
                    const auto metricType = metric.GetMetricType();
                    // static metrics don't get rings
                    if (metricType == PM_METRIC_TYPE_STATIC) {
                        continue;
                    }
                    // TODO: systemize the labelling of metrics that are middleware-derived
                    if (m == PM_METRIC_GPU_FAN_SPEED_PERCENT ||
                        m == PM_METRIC_GPU_MEM_UTILIZATION) {
                        continue;
                    }
                    const auto dataType = metric.GetDataTypeInfo().GetFrameType();
                    gpuShm.GetStore().telemetryData.AddRing(
                        m, telemetryRingSize_, count, dataType
                    );
                }
            }
            void FinalizeGpuDevices() override
            {
                auto lck = LockIntrospectionMutexExclusive_();
                introGpuComplete_ = true;
                if (introGpuComplete_ && introCpuComplete_) {
                    lck.unlock();
                    FinalizeIntrospection_();
                }
            }
            void RegisterCpuDevice(PM_DEVICE_VENDOR vendor,
                std::string deviceName,
                const MetricCapabilities& caps) override
            {
                auto lck = LockIntrospectionMutexExclusive_();
                intro::PopulateCpu(
                    shm_.get_segment_manager(), *pRoot_, vendor, std::move(deviceName), caps
                );
                // populate rings based on caps
                for (auto&& [m, count] : caps) {
                    const auto& metric = pRoot_->FindMetric(m);
                    const auto metricType = metric.GetMetricType();
                    // static metrics don't get rings
                    if (metricType == PM_METRIC_TYPE_STATIC) {
                        continue;
                    }
                    const auto dataType = metric.GetDataTypeInfo().GetFrameType();
                    systemShm_.GetStore().telemetryData.AddRing(
                        m, telemetryRingSize_, count, dataType
                    );
                }
                introCpuComplete_ = true;
                if (introGpuComplete_ && introCpuComplete_) {
                    lck.unlock();
                    FinalizeIntrospection_();
                }
            }
            const ShmNamer& GetNamer() const override
            {
                return namer_;
            }
            // data store access
            std::shared_ptr<OwnedDataSegment<FrameDataStore>>
                CreateOrGetFrameDataSegment(uint32_t pid) override
            {
                // resolve out existing or new weak ptr, try and lock
                auto& pWeak = frameShmWeaks_[pid];
                auto pFrameData = frameShmWeaks_[pid].lock();
                if (!pFrameData) {
                    // if weak ptr was new (or expired), lock will not work and we need to construct
                    // make a frame data store as shared ptr
                    pFrameData = std::make_shared<OwnedDataSegment<FrameDataStore>>(
                        namer_.MakeFrameName(pid), 
                        static_cast<const bip::permissions&>(Permissions_{}),
                        frameRingSize_
                    );
                    // store a weak reference
                    pWeak = pFrameData;
                }
                // remove stale elements to keep map lean
                std::erase_if(frameShmWeaks_, [](auto&&kv){return kv.second.expired();});

                return pFrameData;
            }
            std::shared_ptr<OwnedDataSegment<FrameDataStore>>
                GetFrameDataSegment(uint32_t pid) override
            {
                if (auto i = frameShmWeaks_.find(pid); i != frameShmWeaks_.end()) {
                    if (auto pSegment = i->second.lock()) {
                        return pSegment;
                    }
                    // if weak ptr has expired, garbage collect from the map
                    frameShmWeaks_.erase(i);
                }
                return {};
            }
            std::vector<uint32_t> GetFramePids() const override
            {
                return frameShmWeaks_ | vi::filter([](auto&& p) {return !p.second.expired(); }) |
                    vi::keys | rn::to<std::vector>();
            }
            GpuDataStore& GetGpuDataStore(uint32_t deviceId) override
            {
                const auto it = gpuShms_.find(deviceId);
                if (it == gpuShms_.end()) {
                    pmlog_error("No gpu segment found").pmwatch(deviceId).raise<util::Exception>();
                }
                return it->second.GetStore();
            }
            SystemDataStore& GetSystemDataStore() override
            {
                return systemShm_.GetStore();
            }

        private:
            // types
            class Permissions_
            {
            public:
                Permissions_()
                    :
                    pSecDesc_{ util::win::MakeSecurityDescriptor("D:(A;OICI;GA;;;WD)") },
                    secAttr_{ .nLength = sizeof(secAttr_), .lpSecurityDescriptor = pSecDesc_.get() }
                {}
                operator bip::permissions()
                {
                    return bip::permissions{ &secAttr_ };
                }
            private:
                util::UniqueLocalPtr<void> pSecDesc_;
                SECURITY_ATTRIBUTES secAttr_{ sizeof(secAttr_) };
            };
            // functions
            void PreInitializeIntrospection_()
            {
                // populate introspection data structures at service-side
                auto pSegmentManager = shm_.get_segment_manager();
                auto charAlloc = pSegmentManager->get_allocator<char>();
                intro::PopulateEnums(pSegmentManager, *pRoot_);
                intro::PopulateMetrics(pSegmentManager, *pRoot_);
                intro::PopulateUnits(pSegmentManager, *pRoot_);
                pRoot_->AddDevice(ShmMakeUnique<intro::IntrospectionDevice>(
                    pSegmentManager,
                    0, PM_DEVICE_TYPE_INDEPENDENT, PM_DEVICE_VENDOR_UNKNOWN,
                    ShmString{ "Device-independent", charAlloc }
                ));
            }
            void FinalizeIntrospection_()
            {
                // sort all ordered introspection entities in their principal containers
                pRoot_->Sort();
                // release semaphore holdoff once construction is complete
                for (int i = 0; i < 8; i++) { pIntroSemaphore_->post(); }
            }
            bip::scoped_lock<bip::interprocess_sharable_mutex>
                LockIntrospectionMutexExclusive_()
            {
                const auto result =
                    shm_.find<bip::interprocess_sharable_mutex>(introspectionMutexName_);
                if (!result.first) {
                    throw std::runtime_error{
                        "Failed to find introspection mutex in shared memory"
                    };
                }
                return bip::scoped_lock{ *result.first };
            }

            // data
            ShmNamer namer_;
            ShmSegment shm_;
            ShmUniquePtr<bip::interprocess_sharable_mutex> pIntroMutex_;
            ShmUniquePtr<bip::interprocess_semaphore> pIntroSemaphore_;
            ShmUniquePtr<intro::IntrospectionRoot> pRoot_;
            uint32_t nextDeviceIndex_ = 1;
            bool introGpuComplete_ = false;
            bool introCpuComplete_ = false;

            OwnedDataSegment<SystemDataStore> systemShm_;
            std::unordered_map<uint32_t, std::weak_ptr<OwnedDataSegment<FrameDataStore>>> frameShmWeaks_;
            std::unordered_map<uint32_t, OwnedDataSegment<GpuDataStore>> gpuShms_;
        };

        class MiddlewareComms_ : public MiddlewareComms, CommsBase_
        {
        public:
            MiddlewareComms_(std::string prefix, std::string salt)
                :
                namer_{ std::move(prefix), std::move(salt) },
                shm_{ bip::open_only, namer_.MakeIntrospectionName().c_str() },
                systemShm_{ namer_.MakeSystemName() } // eager-load system segment
            {
                // Eager-load all GPU segments based on introspection
                auto ids = GetGpuDeviceIds_();
                for (auto id : ids) {
                    gpuShms_.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(id),
                        std::forward_as_tuple(namer_.MakeGpuName(id))
                    );
                }
            }
            const PM_INTROSPECTION_ROOT* GetIntrospectionRoot(uint32_t timeoutMs) override
            {
                // make sure holdoff semaphore has been released
                WaitOnIntrospectionHoldoff_(timeoutMs);
                // acquire shared lock on introspection data
                auto sharedLock = LockIntrospectionMutexForShare_();
                // find the introspection structure in shared memory
                const auto result = shm_.find<intro::IntrospectionRoot>(introspectionRootName_);
                if (!result.first) {
                    throw std::runtime_error{ "Failed to find introspection root in shared memory" };
                }
                const auto& root = *result.first;
                // probe allocator used to determine size of memory block required to hold
                // the CAPI introspection structure
                intro::ProbeAllocator<void> probeAllocator;
                // this call to clone doesn't allocate or initialize any memory,
                // the probe just determines required memory
                root.ApiClone(probeAllocator);
                // create actual allocator based on required size
                ipc::intro::BlockAllocator<void> blockAllocator{ probeAllocator.GetTotalSize() };
                // create the CAPI introspection struct on the heap, it is now the caller's
                // responsibility to track this resource
                return root.ApiClone(blockAllocator);
            }
            void OpenFrameDataStore(uint32_t pid) override
            {
                // If already open, nothing to do
                if (frameShms_.find(pid) != frameShms_.end()) {
                    return;
                }

                const auto segName = namer_.MakeFrameName(pid);
                frameShms_.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(pid),
                    std::forward_as_tuple(segName)
                );
            }
            void CloseFrameDataStore(uint32_t pid) override
            {
                frameShms_.erase(pid);
            }
            // data store access
            const FrameDataStore& GetFrameDataStore(uint32_t pid) const override
            {
                const auto it = frameShms_.find(pid);
                if (it == frameShms_.end()) {
                    throw std::runtime_error{ "Frame data segment not open for this PID" };
                }
                return it->second.GetStore();
            }
            const GpuDataStore& GetGpuDataStore(uint32_t deviceId) const override
            {
                const auto it = gpuShms_.find(deviceId);
                if (it == gpuShms_.end()) {
                    throw std::runtime_error{ "No GPU data segment found for this deviceId" };
                }
                return it->second.GetStore();
            }
            const SystemDataStore& GetSystemDataStore() const override
            {
                return systemShm_.GetStore();
            }

        private:
            // functions
            std::vector<uint32_t> GetGpuDeviceIds_()
            {
                // make sure holdoff semaphore has been released
                WaitOnIntrospectionHoldoff_(1500);
                // acquire shared lock on introspection data
                auto sharedLock = LockIntrospectionMutexForShare_();
                // find the introspection structure in shared memory
                const auto result = shm_.find<intro::IntrospectionRoot>(introspectionRootName_);
                if (!result.first) {
                    throw std::runtime_error{ "Failed to find introspection root in shared memory" };
                }
                std::vector<uint32_t> ids;
                for (auto& p : result.first->GetDevices()) {
                    // skip the 0 id
                    if (auto id = p->GetId()) {
                        ids.push_back(id);
                    }
                }
                return ids;
            }
            void WaitOnIntrospectionHoldoff_(uint32_t timeoutMs)
            {
                using namespace std::chrono_literals;
                using clock = std::chrono::high_resolution_clock;
                const auto result = shm_.find<bip::interprocess_semaphore>(introspectionSemaphoreName_);
                if (!result.first) {
                    throw std::runtime_error{
                        "Failed to find introspection semaphore in shared memory"
                    };
                }
                auto& sem = *result.first;
                // wait for holdoff to be released (timeout after timeoutMs)
                if (!sem.timed_wait(clock::now() + 1ms * timeoutMs)) {
                    throw std::runtime_error{ "timeout accessing introspection" };
                }
                // return the slot we just took because holdoff should not limit entry once released
                sem.post();
            }
            bip::sharable_lock<bip::interprocess_sharable_mutex>
                LockIntrospectionMutexForShare_()
            {
                const auto result =
                    shm_.find<bip::interprocess_sharable_mutex>(introspectionMutexName_);
                if (!result.first) {
                    throw std::runtime_error{
                        "Failed to find introspection mutex in shared memory"
                    };
                }
                return bip::sharable_lock{ *result.first };
            }

            // data
            ShmNamer namer_;
            ShmSegment shm_; // introspection shm

            ViewedDataSegment<SystemDataStore> systemShm_;
            std::unordered_map<uint32_t, ViewedDataSegment<GpuDataStore>> gpuShms_;
            std::unordered_map<uint32_t, ViewedDataSegment<FrameDataStore>> frameShms_;
        };
    }

    std::unique_ptr<ServiceComms>
        MakeServiceComms(std::string prefix)
    {
        return std::make_unique<ServiceComms_>(std::move(prefix));
    }

    std::unique_ptr<MiddlewareComms>
        MakeMiddlewareComms(std::string prefix, std::string salt)
    {
        return std::make_unique<MiddlewareComms_>(std::move(prefix), std::move(salt));
    }
}
