#include "../../CommonUtilities/win/WinAPI.h"
#include "Interprocess.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionPopulators.h"
#include "SharedMemoryTypes.h"
#include "OwnedDataSegment.h"
#include "ViewedDataSegment.h"
#include "IntrospectionCloneAllocators.h"
#include "../../CommonUtilities/win/Event.h"
#include "../../CommonUtilities/win/Security.h"
#include "../../CommonUtilities/win/HrError.h"
#include <algorithm>
#include <chrono>
#include "../../PresentMonService/GlobalIdentifiers.h"
#include <windows.h>
#include <sddl.h>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace pmon::ipc
{
    namespace bip = boost::interprocess;
    namespace vi = std::views;
    namespace rn = std::ranges;

    namespace
    {
        util::win::Event MakeIntrospectionReadyEvent_(const std::string& name)
        {
            // Clients only need to wait on the publication event; do not grant modify rights.
            auto pSecDesc = util::win::MakeSecurityDescriptor("D:(A;;0x00100000;;;AU)");
            SECURITY_ATTRIBUTES secAttr{
                .nLength = sizeof(secAttr),
                .lpSecurityDescriptor = pSecDesc.get(),
                .bInheritHandle = FALSE,
            };
            return util::win::Event::CreateNamed(name, true, false, &secAttr);
        }

        void WaitOnIntrospectionReadyEvent_(const util::win::Event& event, uint32_t timeoutMs)
        {
            switch (WaitForSingleObject(event, timeoutMs)) {
            case WAIT_OBJECT_0:
                return;
            case WAIT_TIMEOUT:
                throw std::runtime_error{ "timeout accessing introspection" };
            default:
                throw util::Except<util::win::HrError>("Failed waiting on introspection ready event");
            }
        }

        class CommsBase_
        {
        protected:
            static constexpr size_t introShmSize_ = 0x10'0000;
            static constexpr const char* introspectionRootName_ = "in-root";
        };

        class ServiceComms_ : public ServiceComms, CommsBase_
        {
        public:
            ServiceComms_(std::string prefix,
                size_t frameRingSamples,
                size_t telemetryRingSamples)
                :
                namer_{ std::move(prefix) },
                frameRingSamples_{ std::max(frameRingSamples, kMinRingSamples_) },
                telemetryRingSamples_{ std::max(telemetryRingSamples, kMinRingSamples_) },
                introReadyEvent_{ MakeIntrospectionReadyEvent_(namer_.MakeIntrospectionReadyName()) },
                shm_{ bip::create_only, namer_.MakeIntrospectionName().c_str(),
                    introShmSize_, nullptr, Permissions_{ Permissions_::kReadOnly } },
                pRoot_{ ShmMakeNamedUnique<intro::IntrospectionRoot>(introspectionRootName_,
                    shm_.get_segment_manager(), shm_.get_segment_manager()) }
            {
                PreInitializeIntrospection_();
            }
            intro::IntrospectionRoot& GetIntrospectionRoot() override
            {
                return *pRoot_;
            }
            void RegisterGpuDevice(uint32_t deviceId, PM_DEVICE_VENDOR vendor,
                std::string deviceName, const MetricCapabilities& caps,
                std::span<const uint8_t> luidBytes) override
            {
                std::scoped_lock introLock{ introMutex_ };
                pmlog_dbg("GPU metric capabilities")
                    .pmwatch(deviceId)
                    .pmwatch(deviceName)
                    .pmwatch(caps.ToString(26));
                intro::PopulateGpuDevice(
                    shm_.get_segment_manager(), *pRoot_,
                    deviceId, vendor, deviceName, caps, luidBytes
                );
                const DataStoreSizingInfo sizing{ pRoot_.get().get(), &caps, telemetryRingSamples_ };
                const auto segmentName = namer_.MakeGpuName(deviceId);
                // allocate map node and create shm segment
                auto& gpuShm = gpuShms_.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(deviceId),
                    std::forward_as_tuple(segmentName,
                        sizing,
                        static_cast<const bip::permissions&>(Permissions_{ Permissions_::kReadOnly }))
                ).first->second;
                // populate rings based on caps
                PopulateTelemetryRings(gpuShm.GetStore().telemetryData,
                    sizing,
                    PM_DEVICE_TYPE_GRAPHICS_ADAPTER);
                pmlog_dbg("Shm segment populated (GPU)")
                    .pmwatch(segmentName)
                    .pmwatch(deviceId)
                    .pmwatch(gpuShm.GetBytesTotal())
                    .pmwatch(gpuShm.GetBytesUsed())
                    .pmwatch(gpuShm.GetBytesFree());
            }
            void FinalizeGpuDevices() override
            {
                std::scoped_lock introLock{ introMutex_ };
                introGpuComplete_ = true;
                if (introGpuComplete_ && introCpuComplete_) {
                    FinalizeIntrospection_();
                }
            }
            void RegisterCpuDevice(PM_DEVICE_VENDOR vendor,
                std::string deviceName,
                const MetricCapabilities& caps) override
            {
                std::scoped_lock introLock{ introMutex_ };
                constexpr size_t kWatchIndent = 5;
                constexpr size_t kCapsTextIndent = kWatchIndent + (sizeof("capsText => ") - 1);
                const auto capsText = caps.ToString(kCapsTextIndent);
                pmlog_dbg("CPU metric capabilities")
                    .pmwatch(deviceName)
                    .pmwatch(capsText);
                intro::PopulateCpu(
                    shm_.get_segment_manager(), *pRoot_, vendor, std::move(deviceName), caps
                );
                const DataStoreSizingInfo sizing{ pRoot_.get().get(), &caps, telemetryRingSamples_ };
                const auto segmentName = namer_.MakeSystemName();
                if (!systemShm_) {
                    systemShm_.emplace(segmentName,
                        sizing,
                        static_cast<const bip::permissions&>(Permissions_{ Permissions_::kReadOnly }));
                }
                // populate rings based on caps
                PopulateTelemetryRings(systemShm_->GetStore().telemetryData,
                    sizing,
                    PM_DEVICE_TYPE_SYSTEM);
                pmlog_dbg("Shm segment populated (System)")
                    .pmwatch(segmentName)
                    .pmwatch(systemShm_->GetBytesTotal())
                    .pmwatch(systemShm_->GetBytesUsed())
                    .pmwatch(systemShm_->GetBytesFree());
                introCpuComplete_ = true;
                if (introGpuComplete_ && introCpuComplete_) {
                    FinalizeIntrospection_();
                }
            }
            const ShmNamer& GetNamer() const override
            {
                return namer_;
            }
            // data store access
            std::shared_ptr<OwnedDataSegment<FrameDataStore>>
                CreateOrGetFrameDataSegment(uint32_t pid, bool backpressured) override
            {
                // resolve out existing or new weak ptr, try and lock
                auto& pWeak = frameShmWeaks_[pid];
                auto pFrameData = pWeak.lock();
                if (!pFrameData) {
                    // if weak ptr was new (or expired), lock will not work and we need to construct
                    // make a frame data store as shared ptr
                    const auto segmentName = namer_.MakeFrameName(pid);
                    const DataStoreSizingInfo sizing{
                        .ringSamples = frameRingSamples_,
                        .backpressured = backpressured,
                    };
                    pFrameData = std::shared_ptr<OwnedDataSegment<FrameDataStore>>(
                        new OwnedDataSegment<FrameDataStore>(
                            segmentName,
                            sizing,
                            static_cast<const bip::permissions&>(Permissions_{ Permissions_::kReadOnly })),
                        [pid, segmentName](OwnedDataSegment<FrameDataStore>* pSegment) {
                            pmlog_dbg("Frame data segment destroyed")
                                .pmwatch(pid)
                                .pmwatch(segmentName);
                            delete pSegment;
                        });
                    // store a weak reference
                    pWeak = pFrameData;
                    pmlog_dbg("Frame data segment created")
                        .pmwatch(pid)
                        .pmwatch(segmentName)
                        .pmwatch(backpressured);
                }
                // remove stale elements to keep map lean
                for (auto it = frameShmWeaks_.begin(); it != frameShmWeaks_.end(); ) {
                    if (it->second.expired()) {
                        const auto segmentName = namer_.MakeFrameName(it->first);
                        pmlog_dbg("Frame data segment released")
                            .pmwatch(it->first)
                            .pmwatch(segmentName);
                        it = frameShmWeaks_.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
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
                    const auto segmentName = namer_.MakeFrameName(pid);
                    pmlog_dbg("Frame data segment released")
                        .pmwatch(pid)
                        .pmwatch(segmentName);
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
                if (!systemShm_) {
                    throw std::runtime_error("System data segment not initialized");
                }
                return systemShm_->GetStore();
            }

        private:
            // types
            class Permissions_
            {
            public:
                // Read-only for authenticated users - used for segments clients only read.
                static constexpr const char* kReadOnly  = "D:(A;OICI;GR;;;AU)";

                explicit Permissions_(const char* sddl = kReadOnly)
                    :
                    pSecDesc_{ util::win::MakeSecurityDescriptor(sddl) },
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
                // construct empty LUID object (size = 0 means no LUID)
                auto pLuid = ShmMakeUnique<intro::IntrospectionDeviceLuid>(pSegmentManager, std::span<const uint8_t>{}, pSegmentManager);
                pRoot_->AddDevice(ShmMakeUnique<intro::IntrospectionDevice>(
                    pSegmentManager,
                    0, PM_DEVICE_TYPE_INDEPENDENT, PM_DEVICE_VENDOR_UNKNOWN,
                    ShmString{ "Device-independent", charAlloc }, std::move(pLuid)
                ));
            }
            void FinalizeIntrospection_()
            {
                // sort all ordered introspection entities in their principal containers
                pRoot_->Sort();
                // publish the completed immutable snapshot to readers
                introReadyEvent_.Set();
            }

            // data
            static constexpr size_t kMinRingSamples_ = 8;
            ShmNamer namer_;
            size_t frameRingSamples_;
            size_t telemetryRingSamples_;
            util::win::Event introReadyEvent_;
            std::mutex introMutex_;
            ShmSegment shm_;
            ShmUniquePtr<intro::IntrospectionRoot> pRoot_;
            bool introGpuComplete_ = false;
            bool introCpuComplete_ = false;

            std::optional<OwnedDataSegment<SystemDataStore>> systemShm_;
            std::unordered_map<uint32_t, std::weak_ptr<OwnedDataSegment<FrameDataStore>>> frameShmWeaks_;
            std::unordered_map<uint32_t, OwnedDataSegment<GpuDataStore>> gpuShms_;
        };

        class MiddlewareComms_ : public MiddlewareComms, CommsBase_
        {
        public:
            MiddlewareComms_(std::string prefix, std::string salt)
                :
                namer_{ std::move(prefix), std::move(salt) },
                introReadyEvent_{ util::win::Event::OpenNamed(namer_.MakeIntrospectionReadyName(), SYNCHRONIZE) },
                shm_{ bip::open_read_only, namer_.MakeIntrospectionName().c_str() }
            {
                WaitOnIntrospectionReadyEvent_(introReadyEvent_, 1500);
                systemShm_.emplace(namer_.MakeSystemName());
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
                // make sure the immutable snapshot has been published
                WaitOnIntrospectionReadyEvent_(introReadyEvent_, timeoutMs);
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
                pmlog_dbg("Frame data segment opened")
                    .pmwatch(pid)
                    .pmwatch(segName);
            }
            void CloseFrameDataStore(uint32_t pid) override
            {
                if (auto it = frameShms_.find(pid); it != frameShms_.end()) {
                    const auto segName = namer_.MakeFrameName(pid);
                    pmlog_dbg("Frame data segment closed")
                        .pmwatch(pid)
                        .pmwatch(segName);
                    frameShms_.erase(it);
                }
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
                if (!systemShm_) {
                    throw std::runtime_error{ "System data segment not open" };
                }
                return systemShm_->GetStore();
            }

        private:
            // functions
            std::vector<uint32_t> GetGpuDeviceIds_()
            {
                // make sure the immutable snapshot has been published
                WaitOnIntrospectionReadyEvent_(introReadyEvent_, 1500);
                // find the introspection structure in shared memory
                const auto result = shm_.find<intro::IntrospectionRoot>(introspectionRootName_);
                if (!result.first) {
                    throw std::runtime_error{ "Failed to find introspection root in shared memory" };
                }
                std::vector<uint32_t> ids;
                for (auto& p : result.first->GetDevices()) {
                    // GPU device IDs live in the range (0, kSystemDeviceId)
                    if (auto id = p->GetId(); id > 0 && id < kSystemDeviceId) {
                        ids.push_back(id);
                    }
                }
                return ids;
            }

            // data
            ShmNamer namer_;
            util::win::Event introReadyEvent_;
            ShmSegment shm_; // introspection shm

            std::optional<ViewedDataSegment<SystemDataStore, true>> systemShm_;
            std::unordered_map<uint32_t, ViewedDataSegment<GpuDataStore, true>> gpuShms_;
            std::unordered_map<uint32_t, ViewedDataSegment<FrameDataStore, true>> frameShms_;
        };
    }

    std::unique_ptr<ServiceComms>
        MakeServiceComms(std::string prefix,
            size_t frameRingSamples,
            size_t telemetryRingSamples)
    {
        return std::make_unique<ServiceComms_>(
            std::move(prefix),
            frameRingSamples,
            telemetryRingSamples);
    }

    std::unique_ptr<MiddlewareComms>
        MakeMiddlewareComms(std::string prefix, std::string salt)
    {
        return std::make_unique<MiddlewareComms_>(std::move(prefix), std::move(salt));
    }
}

