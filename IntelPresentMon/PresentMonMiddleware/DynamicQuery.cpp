#include "DynamicQuery.h"
#include "QueryValidation.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../Interprocess/source/SystemDeviceId.h"
#include "../Interprocess/source/Interprocess.h"
#include "../CommonUtilities/Hash.h"
#include <unordered_map>

namespace
{
	struct TelemetryBindingKey_
	{
		uint32_t deviceId;
		PM_METRIC metric;
		uint32_t arrayIndex;

		bool operator==(const TelemetryBindingKey_& other) const noexcept
		{
			return deviceId == other.deviceId &&
				metric == other.metric &&
				arrayIndex == other.arrayIndex;
		}
	};
}

namespace std
{
	template<>
	struct hash<TelemetryBindingKey_>
	{
		size_t operator()(const TelemetryBindingKey_& key) const noexcept
		{
			const size_t h0 = std::hash<uint32_t>{}(key.deviceId);
			const size_t h1 = std::hash<uint32_t>{}((uint32_t)key.metric);
			const size_t h2 = std::hash<uint32_t>{}(key.arrayIndex);
			return pmon::util::hash::HashCombine(pmon::util::hash::HashCombine(h0, h1), h2);
		}
	};
}

namespace pmon::mid::todo
{
	PM_DYNAMIC_QUERY::PM_DYNAMIC_QUERY(std::span<PM_QUERY_ELEMENT> qels, ipc::MiddlewareComms& comms)
	{
		const auto* introBase = comms.GetIntrospectionRoot();
		pmapi::intro::Root introRoot{ introBase, [](const PM_INTROSPECTION_ROOT*) {} };
		pmon::mid::ValidateQueryElements(qels, PM_METRIC_TYPE_DYNAMIC, introRoot, comms);

		std::unordered_map<TelemetryBindingKey_, RingMetricBinding*> telemetryBindings;
		RingMetricBinding* frameBinding = nullptr;

		size_t blobCursor = 0;
		for (auto& qel : qels) {
			RingMetricBinding* binding = nullptr;
			if (qel.deviceId == ipc::kUniversalDeviceId) {
				binding = frameBinding;
				if (!binding) {
					auto bindingPtr = MakeFrameMetricBinding(qel);
					binding = bindingPtr.get();
					frameBinding = bindingPtr.get();
					ringMetricPtrs_.push_back(std::move(bindingPtr));
				}
			}
			else {
				const TelemetryBindingKey_ key{
					.deviceId = qel.deviceId,
					.metric = qel.metric,
					.arrayIndex = qel.arrayIndex,
				};
				if (auto it = telemetryBindings.find(key); it != telemetryBindings.end()) {
					binding = it->second;
				}
				else {
					auto bindingPtr = MakeTelemetryRingMetricBinding(qel, introRoot);
					binding = bindingPtr.get();
					ringMetricPtrs_.push_back(std::move(bindingPtr));
					telemetryBindings.emplace(key, binding);
				}
			}

			qel.dataOffset = blobCursor;
			binding->AddMetricStat(qel, introRoot);
			blobCursor = qel.dataOffset + qel.dataSize;
		}

		for (auto& binding : ringMetricPtrs_) {
			binding->Finalize();
		}

		// make sure blob sizes are multiple of 16 bytes for blob array alignment purposes
		blobSize_ = util::PadToAlignment(blobCursor, 16u);
	}

	size_t PM_DYNAMIC_QUERY::GetBlobSize() const
	{
		return blobSize_;
	}

	DynamicQueryWindow PM_DYNAMIC_QUERY::GenerateQueryWindow_(uint64_t nowTimestamp) const
	{
		return DynamicQueryWindow();
	}


	void PM_DYNAMIC_QUERY::Poll(uint8_t* pBlobBase, ipc::MiddlewareComms& comms,
		uint64_t nowTimestamp, FrameMetricsSource* frameSource) const
	{
		const auto window = GenerateQueryWindow_(nowTimestamp);
		for (auto& pRing : ringMetricPtrs_) {
			pRing->Poll(window, pBlobBase, comms, frameSource);
		}
	}
}
