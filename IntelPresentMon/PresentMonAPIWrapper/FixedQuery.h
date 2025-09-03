#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <IntelPresentMon/Interprocess/source/IntrospectionDataTypeMapping.h>
#include <IntelPresentMon/CommonUtilities/str/String.h>
#include <IntelPresentMon/PresentMonAPIWrapperCommon/EnumMap.h>
#include "Session.h"
#include "BlobContainer.h"
#include <vector>
#include <ranges>
#include <array>
#include <optional>
#include <cassert>


// The FixedQueryContainer gives a simple way to define/register/poll queries and access the results in the specific
// use case where you have a fixed set of metrics you want to poll that is known at compile-time
// Example usage for dynamic query:

//  PM_BEGIN_FIXED_DYNAMIC_QUERY(MyDynamicQuery)
//  	FixedQueryElement fpsAvg{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_AVG };
//  	FixedQueryElement fps90{ this, PM_METRIC_PRESENTED_FPS, PM_STAT_PERCENTILE_90 };
//  	FixedQueryElement presentMode{ this, PM_METRIC_PRESENT_MODE, PM_STAT_MID_POINT };
//  	FixedQueryElement gpuPower{ this, PM_METRIC_GPU_POWER, PM_STAT_AVG, 1 /* poll device at slot1 */ };
//  	FixedQueryElement gpuName{ this, PM_METRIC_GPU_NAME, PM_STAT_NONE, 1 /* poll device at slot1 */ };
//  	FixedQueryElement fanSpeed{ this, PM_METRIC_GPU_FAN_SPEED, PM_STAT_AVG, 1 /* poll device at slot1 */ };
//  PM_END_FIXED_QUERY dq{ session, windowSize, metricOffset, 1 /* nBlobs in container */, 1 /* slot1 maps to device id 1 */};
//  
//  // ... inside polling loop ... assume => void ProcessMetricData(float, float, std::string, float, std::string, float)
//  dq.Poll(processTracker);
//  ProcessMetricData(dq.fpsAvg, dq.fps90, dq.presentMode, dq.gpuPower, dq.gpuName, dq.fanSpeed)

// see SampleClient project for detailed examples of FixedFrameQueryContainer and FixedDynamicQueryContainer


namespace pmapi
{
	// common base for FixedDynamicQueryContainer and FixedStaticQueryContainer
	// NOTE: do not construct instances of this class directly
	struct FixedQueryContainer_
	{
		// give read access to the blob container owned by this fixed query
		const BlobContainer& PeekBlobContainer() const;
		// extracts the blob container owned by this fixed query (query's container is emptied)
		BlobContainer ExtractBlobContainer();
		// injects the blob container into this fixed query (existing one is freed if any)
		void InjectBlobContainer(BlobContainer blobs);
		// swap blob container given with one inside of this fixed query
		void SwapBlobContainers(BlobContainer& blobs);
		// set which blob in the container is active, which controls what data is read when FixeQueryContainer elements are read
		void SetActiveBlobIndex(uint32_t blobIndex);
		// get pointer to the first byte of the active blob
		const uint8_t* PeekActiveBlob() const;
		// get index of the current active blob
		size_t GetActiveBlobIndex() const;
	protected:
		friend class FixedQueryElement;
		// functions
		template<typename...S>
		FixedQueryContainer_(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			pSession_{ &session },
			pIntrospection_{ session.GetIntrospectionRoot() },
			slotDeviceIds_{ uint32_t(slotDeviceIds)... },
			nBlobs_{ nBlobs }
		{}
		void FinalizationPreprocess_();
		uint32_t MapDeviceId_(uint32_t deviceSlot) const;
		void FinalizationPostprocess_(bool isPolled);
		// data
		//   temporary construction storage
		std::vector<uint32_t> slotDeviceIds_;
		std::vector<PM_QUERY_ELEMENT> rawElements_;
		std::vector<class FixedQueryElement*> smartElements_;
		Session* pSession_ = nullptr;
		std::shared_ptr<pmapi::intro::Root> pIntrospection_;
		uint32_t nBlobs_;
		//   retained storage
		BlobContainer blobs_;
		size_t activeBlobIndex_ = 0;
	};
	
	// class for creating a statically-defined query that can automatically register itself
	// and automatically convert query blob data to the desired static type
	// typically not used directly, but rather via the macro PM_BEGIN_FIXED_DYNAMIC_QUERY
	struct FixedDynamicQueryContainer : public FixedQueryContainer_
	{
		// create the fixed dynamic query container
		// nblobs dictates how many swap chains are supported
		// use slotDeviceIds to dictate what device ids will be available to the query elements
		template<typename...S>
		FixedDynamicQueryContainer(Session& session, double winSizeMs, double metricOffsetMs, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			FixedQueryContainer_{ session, nBlobs, slotDeviceIds... },
			winSizeMs_{ winSizeMs },
			metricOffsetMs_{ metricOffsetMs }
		{}
		// create a blob container for this query
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
		// poll this query and populate the container blob container
		void Poll(ProcessTracker& tracker);
	private:
		friend class FinalizingElement;
		// functions
		void Finalize_();
		// data
		//   temporary construction storage
		double winSizeMs_;
		double metricOffsetMs_;
		//   retained storage
		DynamicQuery query_;
	};

	// class for creating a statically-defined query that can automatically register itself
	// and automatically convert query blob data to the desired static type
	// typically not used directly, but rather via the macro PM_BEGIN_FIXED_FRAME_QUERY
	struct FixedFrameQueryContainer : public FixedQueryContainer_
	{
		// create the fixed dynamic query container
		// nblobs dictates how many frames consumed in a single consume call
		// use slotDeviceIds to dictate what device ids will be available to the query elements
		template<typename...S>
		FixedFrameQueryContainer(Session& session, uint32_t nBlobs, S&&...slotDeviceIds)
			:
			FixedQueryContainer_{ session, nBlobs, slotDeviceIds... }
		{}
		// create a blob container for this query
		BlobContainer MakeBlobContainer(uint32_t nBlobs) const;
		// consume frame events and populate the blob container
		void Consume(ProcessTracker& tracker);
		// consume frame events and invoke frameHandler for each frame consumed, setting active blob each time
		// will continue to call consume until all frames have been consumed from the queue
		size_t ForEachConsume(ProcessTracker& tracker, std::function<void()> frameHandler);
	private:
		friend class FinalizingElement;
		// functions
		void Finalize_();
		// data
		FrameQuery query_;
	};

	namespace
	{
		// this static functor converts static types when bridged with runtime PM_DATA_TYPE info
		// enables the |F|ixed |Q|uery elements to choose correct conversion based on both runtime PM_DATA_TYPE
		// and compile-time type of the type to convert to
		template<PM_DATA_TYPE dt, PM_ENUM staticEnumId, typename DestType>
		struct FQReadBridger_
		{
			using SourceType = typename pmon::ipc::intro::DataTypeToStaticType<dt, staticEnumId>::type;
			static void Invoke(PM_ENUM enumId, DestType& dest, const uint8_t* pBlobBytes)
			{
				// void types are not handleable and generally should not occur
				if constexpr (dt == PM_DATA_TYPE_VOID) {
					assert(false && "trying to convert void type");
				}
				// strings (char array) can convert to std::basic_string types
				else if constexpr (dt == PM_DATA_TYPE_STRING) {
					if constexpr (std::same_as<DestType, std::string>) {
						dest = reinterpret_cast<const char*>(pBlobBytes);
					}
					else if constexpr (std::same_as<DestType, std::wstring>) {
						dest = pmon::util::str::ToWide(reinterpret_cast<const char*>(pBlobBytes));
					}
					else {
						assert(false && "failure to convert enum type");
					}
				}
				// enums can convert to numeric types or string types
				else if constexpr (dt == PM_DATA_TYPE_ENUM) {
					if constexpr (std::same_as<DestType, std::string>) {
						try {
							const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
							dest = EnumMap::GetKeyMap(enumId)->at(keyId).narrowName;
						}
						catch (...) { dest = "Invalid"; }
					}
					else if constexpr (std::same_as<DestType, std::wstring>) {
						try {
							const auto keyId = *reinterpret_cast<const int*>(pBlobBytes);
							dest = EnumMap::GetKeyMap(enumId)->at(keyId).wideName;
						}
						catch (...) { dest = L"Invalid"; }
					}
					else if constexpr (
						std::is_integral_v<DestType> ||
						std::is_floating_point_v<DestType> ||
						std::same_as<DestType, SourceType>) {
						dest = DestType(*reinterpret_cast<const int*>(pBlobBytes));
					}
					else {
						assert(false && "failure to convert enum type");
					}
				}
				// what's left is actual numeric types
				else {
					// don't output if the dest is string type
					if constexpr (!std::same_as<DestType, std::string> && !std::same_as<DestType, std::wstring>) {
						dest = static_cast<DestType>(reinterpret_cast<const SourceType&>(*pBlobBytes));
					}
					else {
						assert(false && "failure to convert numeric type");
					}
				}
			}
			static void Default(DestType& dest, const uint8_t* pBlobBytes) {
				assert(false && "failure to convert unknown type");
			}
		};

		// adapter to convert template taking 4 arguments to template taking 2
		// (we need this because you cannot define templates within a function
		// and the static info is only available inside the templated function)
		template<typename T>
		struct FQReadBridgerAdapter_ {
			template<PM_DATA_TYPE dt, PM_ENUM enumId>
			using Bridger = FQReadBridger_<dt, enumId, T>;
		};
	}

	// via macros, you define structs that inherit from a FixedQueryContainer struct and contain one or more FixedQueryElements
	// FixedQueryElement defines what metrics are gathered for the query, and can be accesses to get the results of queries
	// also contains logic to automatically convert to requested types based on the runtime type advertized by
	// PresentMon service introspection
	class FixedQueryElement
	{
		friend FixedQueryContainer_;
	public:
		// create the query element
		// device slot is 1-based index referring to the array of devices passed in with the FixedQueryContainer ctor
		// 1st parameter should be "this" (see example at top of this file)
		FixedQueryElement(FixedQueryContainer_* pContainer, PM_METRIC metric,
			PM_STAT stat, uint32_t deviceSlot = 0, uint32_t index = 0);
		// access this result as a specific static data type, assigning to the pass-in reference
		// this will perform the conversion based on the runtime information about the metric's type
		// enum types will use the introspection system to generate human-readable strings
		// wide/narrow string conversion will also be performed
		template<typename T>
		void Load(T& dest) const
		{
			assert(IsAvailable());
			pmon::ipc::intro::BridgeDataTypeWithEnum<typename FQReadBridgerAdapter_<T>::Bridger>(
				dataType_, enumId_, dest, pContainer_->PeekActiveBlob() + dataOffset_);
		}
		// uses the Load<T> member function to return the converted value as a temporary
		template<typename T>
		T As() const
		{
			T val;
			Load(val);
			return val;
		}
		// uses the As<T> member function for perform implicit conversion
		template<typename T>
		operator T() const
		{
			return As<T>();
		}
		// check if this metric requested by this element is actually available
		// should check this before accessing elements whose metrics may or may not be available
		// e.g. fan speed, gpu voltage, etc.
		bool IsAvailable() const;
		// uses the As<T> member function to return value as optional
		// returned value is empty if IsAvailable() is false
		template<typename T>
		std::optional<T> AsOptional() const
		{
			if (IsAvailable()) {
				return { As<T>() };
			}
			else {
				return std::nullopt;
			}
		}
	private:
		const FixedQueryContainer_* pContainer_ = nullptr;
		uint64_t dataOffset_ = 0ull;
		PM_DATA_TYPE dataType_ = PM_DATA_TYPE_VOID;
		PM_ENUM enumId_ = PM_ENUM_NULL_ENUM;
	};

	// this element should be the last element in a FixedQueryContainer, to trigger compilation processing
	// typically used via the macro PM_END_FIXED_QUERY
	class FinalizingElement
	{
	public:
		template<class E>
		FinalizingElement(E* pContainer)
		{
			pContainer->Finalize_();
		}
	};
}

// begin a fixed dynamic query
#define PM_BEGIN_FIXED_DYNAMIC_QUERY(type) struct type : pmapi::FixedDynamicQueryContainer { using FixedDynamicQueryContainer::FixedDynamicQueryContainer;
// begin a fixed frame query
#define PM_BEGIN_FIXED_FRAME_QUERY(type) struct type : pmapi::FixedFrameQueryContainer { using FixedFrameQueryContainer::FixedFrameQueryContainer;
// end a fixed query (dyanmic or frame)
#define PM_END_FIXED_QUERY private: pmapi::FinalizingElement finalizer{ this }; }