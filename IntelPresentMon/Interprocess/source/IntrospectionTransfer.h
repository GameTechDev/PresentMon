#pragma once
#include <string>
#include <vector>
#include <memory>
#include <span>
#include <ranges>
#include <cstring>
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "SharedMemoryTypes.h"
#include <span>
#include <utility>

namespace pmon::ipc::intro
{
	namespace vi = std::views;

	namespace
	{
		template<typename T>
		concept LessThanComparable_ = requires(T a, T b) {
			{ a < b } -> std::convertible_to<bool>;
		};
	}


	struct IntrospectionString
	{
		IntrospectionString() = delete;
		IntrospectionString(ShmString str)
			:
			buffer_{ std::move(str) }
		{}
		IntrospectionString& operator=(ShmString rhs)
		{
			buffer_ = std::move(rhs);
			return *this;
		}
		using ApiType = PM_INTROSPECTION_STRING;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			using CA = std::allocator_traits<V>::template rebind_alloc<char>;
			CA charAlloc{ voidAlloc };
			const auto bufferSize = buffer_.size() + 1;
			content.pData = charAlloc.allocate(bufferSize);
			if (content.pData) {
				strncpy_s(const_cast<char*>(content.pData), bufferSize, buffer_.c_str(), _TRUNCATE);
			}
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		ShmString buffer_;
	};

	template<typename T>
	struct IntrospectionObjArray
	{
		IntrospectionObjArray(ShmSegmentManager* pSegmentManager) : buffer_{ pSegmentManager->get_allocator<ShmUniquePtr<T>>() } {}
		void PushBack(ShmUniquePtr<T> pObj)
		{
			buffer_.push_back(std::move(pObj));
		}
		using ApiType = PM_INTROSPECTION_OBJARRAY;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			// allocator to construct pointers inside this container
			using VPA = std::allocator_traits<V>::template rebind_alloc<void*>;
			VPA voidPtrAlloc{ voidAlloc };
			content.size = buffer_.size();
			content.pData = const_cast<const void**>(voidPtrAlloc.allocate(content.size));
			// clone each element from shm to Api struct in heap
			for (size_t i = 0; i < content.size; i++) {
				void* pElement = nullptr;
				pElement = buffer_[i]->ApiClone(voidAlloc);
				if (content.pData) {
					content.pData[i] = pElement;
				}
			}
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		std::span<ShmUniquePtr<T>> GetElements()
		{
			return { buffer_ };
		}
		void Sort()
		{
			if constexpr (LessThanComparable_<T>) {
				std::ranges::sort(buffer_, [](auto&& l, auto&& r) { return *l < *r; });
			}
		}
	private:
		ShmVector<ShmUniquePtr<T>> buffer_;
	};

	struct IntrospectionEnumKey
	{
		IntrospectionEnumKey(PM_ENUM enumId_in, int id_in, ShmString symbol_in, ShmString name_in, ShmString shortName_in, ShmString description_in)
			:
			enumId_{ enumId_in},
			id_{ id_in },
			symbol_{ std::move(symbol_in) },
			name_{ std::move(name_in) },
			shortName_{ std::move(shortName_in) },
			description_{ std::move(description_in) }
		{}
		using ApiType = PM_INTROSPECTION_ENUM_KEY;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.enumId = enumId_;
			content.id = id_;
			content.pSymbol = symbol_.ApiClone(voidAlloc);
			content.pName = name_.ApiClone(voidAlloc);
			content.pShortName = shortName_.ApiClone(voidAlloc);
			content.pDescription = description_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		bool operator<(const IntrospectionEnumKey& rhs) const
		{
			return id_ < rhs.id_;
		}
	private:
		PM_ENUM enumId_;
		int id_;
		IntrospectionString symbol_;
		IntrospectionString name_;
		IntrospectionString shortName_;
		IntrospectionString description_;
	};

	struct IntrospectionEnum
	{
		IntrospectionEnum(PM_ENUM id_in, ShmString symbol_in, ShmString description_in)
			:
			id_{ id_in },
			keys_{ symbol_in.get_allocator().get_segment_manager() },
			symbol_{ std::move(symbol_in) },
			description_{ std::move(description_in) }
		{}
		void AddKey(ShmUniquePtr<IntrospectionEnumKey> pKey)
		{
			keys_.PushBack(std::move(pKey));
		}
		using ApiType = PM_INTROSPECTION_ENUM;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.pSymbol = symbol_.ApiClone(voidAlloc);
			content.pDescription = description_.ApiClone(voidAlloc);
			content.pKeys = keys_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		bool operator<(const IntrospectionEnum& rhs) const
		{
			return id_ < rhs.id_;
		}
		void Sort()
		{
			keys_.Sort();
		}
	private:
		PM_ENUM id_;
		IntrospectionObjArray<IntrospectionEnumKey> keys_;
		IntrospectionString symbol_;
		IntrospectionString description_;
	};

	struct IntrospectionDeviceLuid
	{
		IntrospectionDeviceLuid(uint64_t luid, ShmSegmentManager* pSegmentManager)
			:
			buffer_{ pSegmentManager->get_allocator<uint8_t>()}
        {
            buffer_.resize(sizeof(uint64_t));
            std::memcpy(buffer_.data(), &luid, sizeof(uint64_t));
        }
        using ApiType = PM_INTROSPECTION_DEVICE_LUID;
        template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
            // local to hold structure contents being built up
            ApiType content;
            // self allocation
            using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
            A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
            using CA = std::allocator_traits<V>::template rebind_alloc<uint8_t>;
			CA charAlloc{ voidAlloc };
			content.size = static_cast<uint32_t>(buffer_.size());
			content.pData = charAlloc.allocate(content.size);
			if (content.pData) {
				std::memcpy(const_cast<uint8_t*>(content.pData), buffer_.data(), content.size);
			}
            // emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
            return pSelf;
		}
    private:
        ShmVector<uint8_t> buffer_;
	};

	struct IntrospectionDevice
	{
		IntrospectionDevice(uint32_t id_in, PM_DEVICE_TYPE type_in, PM_DEVICE_VENDOR vendor_in, ShmString name_in, ShmUniquePtr<IntrospectionDeviceLuid> pLuid_in)
			:
			id_{ id_in },
			type_{ type_in },
			vendor_{ vendor_in },
			name_{ std::move(name_in) },
			pLuid_{ std::move(pLuid_in) }
		{}
		using ApiType = PM_INTROSPECTION_DEVICE;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.type = type_;
			content.vendor = vendor_;
			content.pName = name_.ApiClone(voidAlloc);
			content.pLuid = pLuid_ ? pLuid_->ApiClone(voidAlloc) : nullptr;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		bool operator<(const IntrospectionDevice& rhs) const
		{
			return id_ < rhs.id_;
		}
	private:
		uint32_t id_;
		PM_DEVICE_TYPE type_;
		PM_DEVICE_VENDOR vendor_;
		IntrospectionString name_;
		ShmUniquePtr<IntrospectionDeviceLuid> pLuid_;
	};

	struct IntrospectionDeviceMetricInfo
	{
		IntrospectionDeviceMetricInfo(uint32_t deviceId_in, PM_METRIC_AVAILABILITY availability_in, uint32_t arraySize_in)
			:
			deviceId_{ deviceId_in },
			availability_{ availability_in },
			arraySize_{ arraySize_in }
		{}
		using ApiType = PM_INTROSPECTION_DEVICE_METRIC_INFO;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.deviceId = deviceId_;
			content.availability = availability_;
			content.arraySize = arraySize_;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		uint32_t deviceId_;
		PM_METRIC_AVAILABILITY availability_;
		uint32_t arraySize_;
	};

	struct IntrospectionDataTypeInfo
	{
		IntrospectionDataTypeInfo(PM_DATA_TYPE polledType_in, PM_DATA_TYPE frameType_in, PM_ENUM enumId_in)
			:
			polledType_{ polledType_in },
			frameType_{ frameType_in },
			enumId_{ enumId_in }
		{}
		using ApiType = PM_INTROSPECTION_DATA_TYPE_INFO;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.polledType = polledType_;
			content.frameType = frameType_;
			content.enumId = enumId_;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		PM_DATA_TYPE polledType_;
		PM_DATA_TYPE frameType_;
		PM_ENUM enumId_;
	};

	struct IntrospectionStatInfo
	{
		IntrospectionStatInfo(PM_STAT stat_in)
			:
			stat_{ stat_in }
		{}
		using ApiType =	PM_INTROSPECTION_STAT_INFO;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.stat = stat_;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
	private:
		PM_STAT stat_;
	};

	struct IntrospectionUnit
	{
		IntrospectionUnit(PM_UNIT id_in, PM_UNIT baseUnit_in, double scale_in)
			:
			id_{ id_in },
			baseUnitId_{ baseUnit_in },
			scale_{ scale_in }
		{}
		using ApiType = PM_INTROSPECTION_UNIT;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.baseUnitId = baseUnitId_;
			content.scale = scale_;
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		bool operator<(const IntrospectionUnit& rhs) const
		{
			return id_ < rhs.id_;
		}
	private:
		PM_UNIT id_;
		PM_UNIT baseUnitId_;
		double scale_;
	};

	struct IntrospectionMetric
	{
		IntrospectionMetric(ShmSegmentManager* pSegmentManager_in, PM_METRIC id_in, PM_METRIC_TYPE type_in, PM_UNIT unit_in, const IntrospectionDataTypeInfo& typeInfo_in, std::vector<PM_STAT> stats_in = {})
			:
			pSegmentManager_{ pSegmentManager_in },
			id_{ id_in },
			type_{ type_in },
			unit_{ unit_in },
			preferredUnitHint_{ unit_in },
			pTypeInfo_{ ShmMakeUnique<IntrospectionDataTypeInfo>(pSegmentManager_in, typeInfo_in) },
			statInfo_{ pSegmentManager_in },
			deviceMetricInfo_{ pSegmentManager_in }
		{
			AddStats(std::move(stats_in));
		}
		void AddStat(PM_STAT stat)
		{
			statInfo_.PushBack(ShmMakeUnique<IntrospectionStatInfo>(pSegmentManager_, stat));
		}
		void AddStats(std::vector<PM_STAT> stats)
		{
			for (auto stat : stats) {
				AddStat(stat);
			}
		}
		void AddDeviceMetricInfo(IntrospectionDeviceMetricInfo info)
		{
			deviceMetricInfo_.PushBack(ShmMakeUnique<IntrospectionDeviceMetricInfo>(pSegmentManager_, info));
		}
		void SetPreferredUnitHint(PM_UNIT hint)
		{
			preferredUnitHint_ = hint;
		}
		using ApiType = PM_INTROSPECTION_METRIC;
		template<class V>
		ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// prepare contents
			content.id = id_;
			content.type = type_;
			content.unit = unit_;
			content.preferredUnitHint = preferredUnitHint_;
			content.pTypeInfo = pTypeInfo_->ApiClone(voidAlloc);
			content.pStatInfo = statInfo_.ApiClone(voidAlloc);
			content.pDeviceMetricInfo = deviceMetricInfo_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		PM_METRIC GetId() const
		{
			return id_;
		}
		bool operator<(const IntrospectionMetric& rhs) const
		{
			return id_ < rhs.id_;
		}
	private:
		ShmSegmentManager* pSegmentManager_;
		PM_METRIC id_;
		PM_METRIC_TYPE type_;
		PM_UNIT unit_;
		PM_UNIT preferredUnitHint_;
		IntrospectionObjArray<IntrospectionStatInfo> statInfo_;
		IntrospectionObjArray<IntrospectionDeviceMetricInfo> deviceMetricInfo_;
		ShmUniquePtr<IntrospectionDataTypeInfo> pTypeInfo_;
	};

	struct IntrospectionRoot
	{
		IntrospectionRoot(ShmSegmentManager* pSegmentManager_in)
			:
			metrics_{ pSegmentManager_in },
			enums_{ pSegmentManager_in },
			devices_{ pSegmentManager_in },
			units_ { pSegmentManager_in }
		{}
		void AddEnum(ShmUniquePtr<IntrospectionEnum> pEnum)
		{
			enums_.PushBack(std::move(pEnum));
		}
		void AddMetric(ShmUniquePtr<IntrospectionMetric> pMetric)
		{
			metrics_.PushBack(std::move(pMetric));
		}
		void AddDevice(ShmUniquePtr<IntrospectionDevice> pDevice)
		{
			devices_.PushBack(std::move(pDevice));
		}
		void AddUnit(ShmUniquePtr<IntrospectionUnit> pUnit)
		{
			units_.PushBack(std::move(pUnit));
		}
		std::span<ShmUniquePtr<IntrospectionMetric>> GetMetrics()
		{
			return metrics_.GetElements();
		}
		using ApiType = PM_INTROSPECTION_ROOT;
		template<class V>
		const ApiType* ApiClone(V voidAlloc) const
		{
			// local to hold structure contents being built up
			ApiType content;
			// self allocation
			using A = std::allocator_traits<V>::template rebind_alloc<ApiType>;
			A alloc{ voidAlloc };
			auto pSelf = alloc.allocate(1);
			// TODO: prepare contents
			content.pMetrics = metrics_.ApiClone(voidAlloc);
			content.pEnums = enums_.ApiClone(voidAlloc);
			content.pDevices = devices_.ApiClone(voidAlloc);
			content.pUnits = units_.ApiClone(voidAlloc);
			// emplace to allocated self
			if (pSelf) {
				std::allocator_traits<A>::construct(alloc, pSelf, content);
			}
			return pSelf;
		}
		void Sort()
		{
			metrics_.Sort();
			enums_.Sort();
			devices_.Sort();
			for (auto& pe : enums_.GetElements()) {
				pe->Sort();
			}
			units_.Sort();
		}
	private:
		IntrospectionObjArray<IntrospectionMetric> metrics_;
		IntrospectionObjArray<IntrospectionEnum> enums_;
		IntrospectionObjArray<IntrospectionDevice> devices_;
		IntrospectionObjArray<IntrospectionUnit> units_;
	};
}