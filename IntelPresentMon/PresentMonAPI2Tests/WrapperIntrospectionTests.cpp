#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include <boost/process/v1/child.hpp>
#include "../PresentMonAPI2/Internal.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPI2Loader/Loader.h"
#include "../Versioning/BuildId.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace bp = boost::process;
using namespace std::string_literals;
using namespace std::chrono_literals;
namespace rn = std::ranges;
namespace vi = rn::views;
using namespace pmapi;

namespace EndToEndTests
{
	TEST_CLASS(WrapperServiceTests)
	{
		std::string ctlPipeName = R"(\\.\pipe\test-pipe-pmsvc-2)"s;
		std::string introName = "PM_intro_test_nsm_2"s;
		std::optional<boost::process::v1::child> oChild;
		std::optional<Session> oSession;
		std::shared_ptr<intro::Root> pData;
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			if (pData) {
				pData.reset();
			}
			if (oSession) {
				oSession.reset();
			}
			if (oChild) {
				oChild->terminate();
				oChild->wait();
				oChild.reset();
			}
		}
		TEST_METHOD_INITIALIZE(Init)
		{
			oChild.emplace("PresentMonService.exe"s,
				"--timed-stop"s, "4000"s,
				"--control-pipe"s, ctlPipeName.c_str(),
				"--nsm-prefix"s, "pmon_nsm_utest_"s,
				"--intro-nsm"s, introName);

			std::this_thread::sleep_for(100ms);

			pmLoaderSetPathToMiddlewareDll_("./PresentMonAPI2.dll");
			pmSetupODSLogging_(PM_DIAGNOSTIC_LEVEL_DEBUG, PM_DIAGNOSTIC_LEVEL_ERROR, false);
			oSession.emplace(ctlPipeName);

			pData = oSession->GetIntrospectionRoot();
		}

		TEST_METHOD(IntrospectMetaEnumFullSet)
		{
			// Question: why are these not like "PM_ENUM_STATUS"?
			const std::vector expected{
				"PM_STATUS"s,
				"PM_METRIC"s,
				"PM_METRIC_TYPE"s,
				"PM_DEVICE_VENDOR"s,
				"PM_PRESENT_MODE"s,
				"PM_UNIT"s,
				"PM_STAT"s,
				"PM_DATA_TYPE"s,
				"PM_GRAPHICS_RUNTIME"s,
				"PM_FRAME_TYPE"s,
				"PM_DEVICE_TYPE"s,
				"PM_METRIC_AVAILABILITY"s,
				"PM_NULL_ENUM"s,
			};
			for (auto&& [expected, actual] : vi::zip(expected, pData->GetEnums())) {
				Assert::AreEqual(expected, actual.GetSymbol());
			}
		}
		TEST_METHOD(IntrospectEnumKeyBySymbols)
		{
			auto unit = rn::find(pData->GetEnums(), "PM_UNIT"s, &intro::EnumView::GetSymbol);
			auto millisec = rn::find(unit->GetKeys(), "PM_UNIT_MILLISECONDS"s, &intro::EnumKeyView::GetSymbol);
			Assert::AreEqual("ms"s, millisec->GetShortName());
		}
		TEST_METHOD(IntrospectUnitById)
		{
			for (auto u : pData->GetUnits()) {
				auto id = u->GetId();
				int x = 0;
			}
			auto fps = rn::find(pData->GetUnits(), PM_UNIT_FPS, &intro::UnitView::GetId);
			Assert::IsFalse(fps == pData->GetUnits().end());
			Assert::AreEqual("fps"s, fps->Introspect()->GetShortName());
		}
		TEST_METHOD(IntrospectMetricByName)
		{
			auto gpuName = rn::find_if(pData->GetMetrics(),
				[](auto&& e) { return e.GetName() == "GPU Name"s; },
				&intro::MetricView::Introspect);
			Assert::AreEqual((int)PM_METRIC_GPU_NAME, (int)gpuName->GetId());
		}
		TEST_METHOD(IntrospectMetricStats)
		{
			auto swap = rn::find(pData->GetMetrics(), PM_METRIC_SWAP_CHAIN_ADDRESS, &intro::MetricView::GetId);
			Assert::IsFalse(swap == pData->GetMetrics().end());
			Assert::AreEqual("PM_STAT_MID_POINT"s, swap->GetStatInfo().begin()->IntrospectStat()->GetSymbol());
		}
		TEST_METHOD(IntrospectMetricDataTypeEnum)
		{
			auto metric = rn::find(pData->GetMetrics(), PM_METRIC_PRESENT_MODE, &intro::MetricView::GetId);
			auto type = metric->GetDataTypeInfo();
			Assert::AreEqual("Present Mode"s, metric->Introspect().GetName());
			Assert::AreEqual((int)PM_DATA_TYPE_ENUM, (int)type.GetPolledType());
			Assert::AreEqual((int)PM_DATA_TYPE_ENUM, (int)type.GetFrameType());
			Assert::AreEqual("PM_PRESENT_MODE"s, type.IntrospectEnum().GetSymbol());
		}
		TEST_METHOD(IntrospectMetricDataTypeDivergent)
		{
			auto metric = rn::find(pData->GetMetrics(), PM_METRIC_GPU_POWER_LIMITED, &intro::MetricView::GetId);
			auto type = metric->GetDataTypeInfo();
			Assert::AreEqual("GPU Power Limited"s, metric->Introspect().GetName());
			Assert::AreEqual((int)PM_DATA_TYPE_DOUBLE, (int)type.GetPolledType());
			Assert::AreEqual((int)PM_DATA_TYPE_BOOL, (int)type.GetFrameType());
		}
		//TEST_METHOD(IntrospectMetricDeviceMetricInfo)
		//{
		//	using namespace std::string_literals;
		//	auto metric = data->FindMetric(PM_METRIC_GPU_FAN_SPEED);
		//	auto deviceInfos = metric.GetDeviceMetricInfo();
		//	Assert::AreEqual(2ull, deviceInfos.size());
		//	{
		//		auto deviceInfo = deviceInfos.begin()[0];
		//		Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)deviceInfo.GetAvailability());
		//		Assert::AreEqual(1u, deviceInfo.GetArraySize());
		//	}
		//	{
		//		auto deviceInfo = deviceInfos.begin()[1];
		//		Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)deviceInfo.GetAvailability());
		//		Assert::AreEqual(2u, deviceInfo.GetArraySize());
		//	}
		//}
		//TEST_METHOD(IntrospectDevices)
		//{
		//	using namespace std::string_literals;
		//	auto devices = data->GetDevices();
		//	Assert::AreEqual(3ull, devices.size());
		//	{
		//		auto device = devices.begin()[0];
		//		Assert::AreEqual("Unknown"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("Device-independent"s, device.GetName());
		//		Assert::AreEqual("Device Independent"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(0u, device.GetId());
		//	}
		//	{
		//		auto device = devices.begin()[1];
		//		Assert::AreEqual("Intel"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("Arc 750"s, device.GetName());
		//		Assert::AreEqual("Graphics Adapter"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(1u, device.GetId());
		//	}
		//	{
		//		auto device = devices.begin()[2];
		//		Assert::AreEqual("NVIDIA"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("GeForce RTX 2080 ti"s, device.GetName());
		//		Assert::AreEqual("Graphics Adapter"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(2u, device.GetId());
		//	}
		//}
		//TEST_METHOD(IntrospectLookupDevice)
		//{
		//	using namespace std::string_literals;
		//	{
		//		auto device = data->FindDevice(1);
		//		Assert::AreEqual("Intel"s, device.IntrospectVendor().GetName());
		//		Assert::AreEqual("Arc 750"s, device.GetName());
		//		Assert::AreEqual("Graphics Adapter"s, device.IntrospectType().GetName());
		//		Assert::AreEqual(1u, device.GetId());
		//	}
		//}
		//TEST_METHOD(IntrospectMetricDeviceMetricInfoLookupDevice)
		//{
		//	using namespace std::string_literals;
		//	auto metric = data->FindMetric(PM_METRIC_GPU_FAN_SPEED);
		//	auto deviceInfos = metric.GetDeviceMetricInfo();
		//	Assert::AreEqual(2ull, deviceInfos.size());
		//	{
		//		auto deviceInfo = deviceInfos.begin()[1];
		//		Assert::AreEqual((int)PM_METRIC_AVAILABILITY_AVAILABLE, (int)deviceInfo.GetAvailability());
		//		Assert::IsTrue(deviceInfo.IsAvailable());
		//		Assert::AreEqual(2u, deviceInfo.GetArraySize());
		//		Assert::AreEqual("NVIDIA"s, deviceInfo.GetDevice().IntrospectVendor().GetName());
		//	}
		//}
		TEST_METHOD(IntrospectMetricType)
		{
			auto cpuUtilMetric = rn::find(pData->GetMetrics(), PM_METRIC_CPU_UTILIZATION, &intro::MetricView::GetId);
			Assert::AreEqual("Dynamic and Frame Event Metric"s, cpuUtilMetric->IntrospectType().GetName());
			auto appMetric = rn::find(pData->GetMetrics(), PM_METRIC_APPLICATION, &intro::MetricView::GetId);
			Assert::AreEqual("Static Metric"s, appMetric->IntrospectType().GetName());
		}
		TEST_METHOD(IntrospectStat)
		{
			auto statEnum = rn::find(pData->GetEnums(), PM_ENUM_STAT, &intro::EnumView::GetId);
			auto avg = rn::find(statEnum->GetKeys(), PM_STAT_AVG, &intro::EnumKeyView::GetId);
			Assert::AreEqual("avg"s, avg->GetShortName());
		}
		TEST_METHOD(IntrospectPreferredUnit)
		{
			auto metric = rn::find(pData->GetMetrics(), PM_METRIC_GPU_MEM_SIZE, &intro::MetricView::GetId);
			Assert::AreEqual((int)PM_UNIT_BYTES, (int)metric->GetUnit());
			Assert::AreEqual((int)PM_UNIT_GIGABYTES, (int)metric->GetPreferredUnitHint());
		}
		TEST_METHOD(IntrospectUnitUpConversion)
		{
			auto unit = rn::find(pData->GetUnits(), PM_UNIT_HERTZ, &intro::UnitView::GetId);
			Assert::AreEqual(0.000'001, unit->MakeConversionFactor(PM_UNIT_MEGAHERTZ));
		}
		TEST_METHOD(CApiVersion)
		{
			PM_VERSION ver;
			Assert::AreEqual(PM_STATUS_SUCCESS, pmGetApiVersion(&ver));
			Assert::AreEqual(3, (int)ver.major);
			Assert::AreEqual(0, (int)ver.minor);
			Assert::AreEqual(0, (int)ver.patch);
			Assert::AreEqual("", ver.tag);
			Assert::AreEqual(pmon::bid::BuildIdShortHash(), ver.hash);
		}
	};
}