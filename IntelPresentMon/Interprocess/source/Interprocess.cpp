#include "Interprocess.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionPopulators.h"
#include "SharedMemoryTypes.h"
#include "IntrospectionCloneAllocators.h"
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <chrono>
#include "../../PresentMonService/GlobalIdentifiers.h"
#include <windows.h>
#include <sddl.h>

namespace pmon::ipc
{
	namespace bip = boost::interprocess;

	namespace
	{
		class CommsBase_
		{
		protected:
			static constexpr const char* defaultSegmentName_ = pmon::gid::defaultIntrospectionNsmName;
			static constexpr const char* introspectionRootName_ = "in-root";
			static constexpr const char* introspectionMutexName_ = "in-mtx";
			static constexpr const char* introspectionSemaphoreName_ = "in-sem";
		};

		class ServiceComms_ : public ServiceComms, CommsBase_
		{
		public:
			ServiceComms_(std::optional<std::string> sharedMemoryName)
				:
				shm_{ bip::create_only, sharedMemoryName.value_or(defaultSegmentName_).c_str(),
					0x10'0000, nullptr, Permissions_{} },
				pIntroMutex_{ ShmMakeNamedUnique<bip::interprocess_sharable_mutex>(
					introspectionMutexName_, shm_.get_segment_manager()) },
				pIntroSemaphore_{ ShmMakeNamedUnique<bip::interprocess_semaphore>(
					introspectionSemaphoreName_, shm_.get_segment_manager(), 0) },
				pRoot_{ ShmMakeNamedUnique<intro::IntrospectionRoot>(introspectionRootName_,
					shm_.get_segment_manager(), shm_.get_segment_manager()) }
			{
				PreInitializeIntrospection_();
			}
			intro::IntrospectionRoot& GetIntrospectionRoot() override
			{
				return *pRoot_;
			}
			void RegisterGpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const GpuTelemetryBitset& gpuCaps) override
			{
				auto lck = LockIntrospectionMutexExclusive_();
				intro::PopulateGpuDevice(shm_.get_segment_manager(), *pRoot_, nextDeviceIndex_++, vendor, deviceName, gpuCaps);
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
			void RegisterCpuDevice(PM_DEVICE_VENDOR vendor, std::string deviceName, const CpuTelemetryBitset& cpuCaps) override
			{
				auto lck = LockIntrospectionMutexExclusive_();
				intro::PopulateCpu(shm_.get_segment_manager(), *pRoot_, vendor, deviceName, cpuCaps);
				introCpuComplete_ = true;
				if (introGpuComplete_ && introCpuComplete_) {
					lck.unlock();
					FinalizeIntrospection_();
				}
			}
		private:
			// types
			class Permissions_
			{
			public:
				Permissions_()
				{
					if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
						"D:(A;OICI;GA;;;WD)",
						SDDL_REVISION_1, &secAttr_.lpSecurityDescriptor, NULL)) {
						throw std::runtime_error{ "Failed to create security descriptor for shared memory" };
					}
				}
				operator bip::permissions()
				{
					return bip::permissions{ &secAttr_ };
				}
			private:
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
				pRoot_->AddDevice(ShmMakeUnique<intro::IntrospectionDevice>(pSegmentManager,
					0, PM_DEVICE_TYPE_INDEPENDENT, PM_DEVICE_VENDOR_UNKNOWN, ShmString{ "Device-independent", charAlloc }));
			}
			void FinalizeIntrospection_()
			{
				// sort all ordered introspection entities in their pricipal containers
				pRoot_->Sort();
				// release semaphore holdoff once construction is complete
				for (int i = 0; i < 8; i++) { pIntroSemaphore_->post(); }
			}
			bip::scoped_lock<bip::interprocess_sharable_mutex> LockIntrospectionMutexExclusive_()
			{
				const auto result = shm_.find<bip::interprocess_sharable_mutex>(introspectionMutexName_);
				if (!result.first) {
					throw std::runtime_error{ "Failed to find introspection mutex in shared memory" };
				}
				return bip::scoped_lock{ *result.first };
			}
			// data
			ShmSegment shm_;
			ShmUniquePtr<bip::interprocess_sharable_mutex> pIntroMutex_;
			ShmUniquePtr<bip::interprocess_semaphore> pIntroSemaphore_;
			ShmUniquePtr<intro::IntrospectionRoot> pRoot_;
			uint32_t nextDeviceIndex_ = 1;
			bool introGpuComplete_ = false;
			bool introCpuComplete_ = false;
		};

		class MiddlewareComms_ : public MiddlewareComms, CommsBase_
		{
		public:
			MiddlewareComms_(std::optional<std::string> sharedMemoryName)
				:
				shm_{ bip::open_only, sharedMemoryName.value_or(defaultSegmentName_).c_str() }
			{}
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
				// probe allocator used to determine size of memory block required to hold the CAPI instrospection structure
				intro::ProbeAllocator<void> probeAllocator;
				// this call to clone doesn't allocate of initialize any memory, the probe just determines required memory
				root.ApiClone(probeAllocator);
				// create actual allocator based on required size
				ipc::intro::BlockAllocator<void> blockAllocator{ probeAllocator.GetTotalSize() };
				// create the CAPI introspection struct on the heap, it is now the caller's responsibility to track this resource
				return root.ApiClone(blockAllocator);
			}
		private:
			// functions
			void WaitOnIntrospectionHoldoff_(uint32_t timeoutMs)
			{
				using namespace std::chrono_literals;
				using clock = std::chrono::high_resolution_clock;
				const auto result = shm_.find<bip::interprocess_semaphore>(introspectionSemaphoreName_);
				if (!result.first) {
					throw std::runtime_error{ "Failed to find introspection semaphore in shared memory" };
				}
				auto& sem = *result.first;
				// wait for holdoff to be released (timeout after XXXms)
				if (!sem.timed_wait(clock::now() + 1ms * timeoutMs)) {
					throw std::runtime_error{ "timeout accessing introspection" };
				}
				// return the slot we just took because holdoff should not limit entry once released
				sem.post();
			}
			bip::sharable_lock<bip::interprocess_sharable_mutex> LockIntrospectionMutexForShare_()
			{
				const auto result = shm_.find<bip::interprocess_sharable_mutex>(introspectionMutexName_);
				if (!result.first) {
					throw std::runtime_error{ "Failed to find introspection mutex in shared memory" };
				}
				return bip::sharable_lock{ *result.first };
			}
			// data
			ShmSegment shm_;
		};
	}

	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::optional<std::string> sharedMemoryName)
	{
		return std::make_unique<MiddlewareComms_>(std::move(sharedMemoryName));
	}

	std::unique_ptr<ServiceComms> MakeServiceComms(std::optional<std::string> sharedMemoryName)
	{
		return std::make_unique<ServiceComms_>(std::move(sharedMemoryName));
	}
}