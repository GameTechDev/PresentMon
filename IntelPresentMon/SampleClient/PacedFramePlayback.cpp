#include "PacedFramePlayback.h"
#include "CliOptions.h"
#include <PresentMonAPIWrapperCommon/EnumMap.h>
#include <PresentMonAPIWrapper/FixedQuery.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/log/Log.h>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

using namespace pmon;
using namespace std::literals;

namespace
{
	const std::array<const char*, 31> kFrameCsvHeader{
		"Application",
		"ProcessID",
		"SwapChainAddress",
		"PresentRuntime",
		"SyncInterval",
		"PresentFlags",
		"AllowsTearing",
		"PresentMode",
		"FrameType",
		"CPUStartTime",
		"MsBetweenSimulationStart",
		"MsBetweenPresents",
		"MsBetweenDisplayChange",
		"MsInPresentAPI",
		"MsRenderPresentLatency",
		"MsUntilDisplayed",
		"MsPCLatency",
		"MsBetweenAppStart",
		"MsCPUBusy",
		"MsCPUWait",
		"MsGPULatency",
		"MsGPUTime",
		"MsGPUBusy",
		"MsGPUWait",
		"MsVideoBusy",
		"MsAnimationError",
		"AnimationTime",
		"MsFlipDelay",
		"MsAllInputToPhotonLatency",
		"MsClickToPhotonLatency",
		"MsInstrumentedLatency",
	};

	std::string TranslateGraphicsRuntime(PM_GRAPHICS_RUNTIME runtime)
	{
		switch (runtime) {
		case PM_GRAPHICS_RUNTIME_DXGI:
			return "DXGI";
		case PM_GRAPHICS_RUNTIME_D3D9:
			return "D3D9";
		default:
			return "Other";
		}
	}

	std::string TranslatePresentMode(PM_PRESENT_MODE presentMode)
	{
		switch (presentMode) {
		case PM_PRESENT_MODE_HARDWARE_LEGACY_FLIP:
			return "Hardware: Legacy Flip";
		case PM_PRESENT_MODE_HARDWARE_LEGACY_COPY_TO_FRONT_BUFFER:
			return "Hardware: Legacy Copy to front buffer";
		case PM_PRESENT_MODE_HARDWARE_INDEPENDENT_FLIP:
			return "Hardware: Independent Flip";
		case PM_PRESENT_MODE_COMPOSED_FLIP:
			return "Composed: Flip";
		case PM_PRESENT_MODE_HARDWARE_COMPOSED_INDEPENDENT_FLIP:
			return "Hardware Composed: Independent Flip";
		case PM_PRESENT_MODE_COMPOSED_COPY_WITH_GPU_GDI:
			return "Composed: Copy with GPU GDI";
		case PM_PRESENT_MODE_COMPOSED_COPY_WITH_CPU_GDI:
			return "Composed: Copy with CPU GDI";
		default:
			return "Other";
		}
	}

	std::string TranslateFrameType(PM_FRAME_TYPE frameType)
	{
		switch (frameType) {
		case PM_FRAME_TYPE_NOT_SET:
		case PM_FRAME_TYPE_UNSPECIFIED:
		case PM_FRAME_TYPE_APPLICATION:
			return "Application";
		case PM_FRAME_TYPE_AMD_AFMF:
			return "AMD_AFMF";
		case PM_FRAME_TYPE_INTEL_XEFG:
			return "Intel XeSS-FG";
		default:
			return "Other";
		}
	}

	void WriteHeader(std::ofstream& csv)
	{
		for (size_t i = 0; i < kFrameCsvHeader.size(); ++i) {
			if (i > 0) {
				csv << ",";
			}
			csv << kFrameCsvHeader[i];
		}
		csv << "\n";
	}

	void WriteOptionalDouble(std::ofstream& csv, double value)
	{
		if (std::isnan(value)) {
			csv << "NA";
			return;
		}
		csv << value;
	}

	void WriteOptionalElement(std::ofstream& csv, const pmapi::FixedQueryElement& element)
	{
		if (!element.IsAvailable()) {
			csv << "NA";
			return;
		}
		WriteOptionalDouble(csv, element.As<double>());
	}
}

int PacedFramePlaybackTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();

	std::optional<PM_STATUS> errorStatus;

	try {
		if (!opt.processId) {
			pmlog_error("need pid");
		}
		if (!opt.outputPath) {
			pmlog_error("need output path");
		}

		if (opt.etwFlushPeriodMs) {
			pSession->SetEtwFlushPeriod(*opt.etwFlushPeriodMs);
		}
		if (opt.telemetryPeriodMs) {
			pSession->SetTelemetryPollingPeriod(0, *opt.telemetryPeriodMs);
		}

		std::string line;
		std::getline(std::cin, line);
		if (line != "%ping") {
			std::cout << "%%{ping-error}%%" << std::endl;
			return -1;
		}
		std::cout << "%%{ping-ok}%%" << std::endl;

		const auto processName = opt.processName.AsOptional().value_or("unknown"s);
		const auto frameLimit = static_cast<size_t>(*opt.frameLimit);

		PM_BEGIN_FIXED_FRAME_QUERY(FrameQuery)
			pmapi::FixedQueryElement swapChain{ this, PM_METRIC_SWAP_CHAIN_ADDRESS, PM_STAT_NONE };
			pmapi::FixedQueryElement presentRuntime{ this, PM_METRIC_PRESENT_RUNTIME, PM_STAT_NONE };
			pmapi::FixedQueryElement syncInterval{ this, PM_METRIC_SYNC_INTERVAL, PM_STAT_NONE };
			pmapi::FixedQueryElement presentFlags{ this, PM_METRIC_PRESENT_FLAGS, PM_STAT_NONE };
			pmapi::FixedQueryElement allowsTearing{ this, PM_METRIC_ALLOWS_TEARING, PM_STAT_NONE };
			pmapi::FixedQueryElement presentMode{ this, PM_METRIC_PRESENT_MODE, PM_STAT_NONE };
			pmapi::FixedQueryElement frameType{ this, PM_METRIC_FRAME_TYPE, PM_STAT_NONE };
			pmapi::FixedQueryElement cpuStartTime{ this, PM_METRIC_CPU_START_TIME, PM_STAT_NONE };
			pmapi::FixedQueryElement msBetweenSimStart{ this, PM_METRIC_BETWEEN_SIMULATION_START, PM_STAT_NONE };
			pmapi::FixedQueryElement msBetweenPresents{ this, PM_METRIC_BETWEEN_PRESENTS, PM_STAT_NONE };
			pmapi::FixedQueryElement msBetweenDisplayChange{ this, PM_METRIC_BETWEEN_DISPLAY_CHANGE, PM_STAT_NONE };
			pmapi::FixedQueryElement msInPresentApi{ this, PM_METRIC_IN_PRESENT_API, PM_STAT_NONE };
			pmapi::FixedQueryElement msRenderPresentLatency{ this, PM_METRIC_RENDER_PRESENT_LATENCY, PM_STAT_NONE };
			pmapi::FixedQueryElement msUntilDisplayed{ this, PM_METRIC_UNTIL_DISPLAYED, PM_STAT_NONE };
			pmapi::FixedQueryElement msPcLatency{ this, PM_METRIC_PC_LATENCY, PM_STAT_NONE };
			pmapi::FixedQueryElement msBetweenAppStart{ this, PM_METRIC_BETWEEN_APP_START, PM_STAT_NONE };
			pmapi::FixedQueryElement msCpuBusy{ this, PM_METRIC_CPU_BUSY, PM_STAT_NONE };
			pmapi::FixedQueryElement msCpuWait{ this, PM_METRIC_CPU_WAIT, PM_STAT_NONE };
			pmapi::FixedQueryElement msGpuLatency{ this, PM_METRIC_GPU_LATENCY, PM_STAT_NONE };
			pmapi::FixedQueryElement msGpuTime{ this, PM_METRIC_GPU_TIME, PM_STAT_NONE };
			pmapi::FixedQueryElement msGpuBusy{ this, PM_METRIC_GPU_BUSY, PM_STAT_NONE };
			pmapi::FixedQueryElement msGpuWait{ this, PM_METRIC_GPU_WAIT, PM_STAT_NONE };
			pmapi::FixedQueryElement msAnimationError{ this, PM_METRIC_ANIMATION_ERROR, PM_STAT_NONE };
			pmapi::FixedQueryElement animationTime{ this, PM_METRIC_ANIMATION_TIME, PM_STAT_NONE };
			pmapi::FixedQueryElement msFlipDelay{ this, PM_METRIC_FLIP_DELAY, PM_STAT_NONE };
			pmapi::FixedQueryElement msAllInputToPhotonLatency{ this, PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY, PM_STAT_NONE };
			pmapi::FixedQueryElement msClickToPhotonLatency{ this, PM_METRIC_CLICK_TO_PHOTON_LATENCY, PM_STAT_NONE };
			pmapi::FixedQueryElement msInstrumentedLatency{ this, PM_METRIC_INSTRUMENTED_LATENCY, PM_STAT_NONE };
		PM_END_FIXED_QUERY query{ *pSession, 512 };

		auto tracker = pSession->TrackProcess(*opt.processId);

		std::ofstream csv{ *opt.outputPath };
		if (!csv.is_open()) {
			pmlog_error("failed to open output file");
			return -1;
		}
		csv << std::fixed << std::setprecision(14);
		WriteHeader(csv);

		using Clock = std::chrono::high_resolution_clock;
		const auto start = Clock::now();
		size_t emptyPollCount = 0;
		size_t totalRecorded = 0;
		const size_t emptyLimit = 10;

		while (true) {
			const auto elapsed = std::chrono::duration<double>(Clock::now() - start).count();
			const auto processed = query.ForEachConsume(tracker, [&] {
				if (frameLimit > 0 && totalRecorded >= frameLimit) {
					return;
				}
				csv << processName << ",";
				csv << *opt.processId << ",";
				csv << std::hex << std::uppercase << "0x" << query.swapChain.As<uint64_t>()
					<< std::dec << std::nouppercase << ",";
				csv << TranslateGraphicsRuntime(query.presentRuntime.As<PM_GRAPHICS_RUNTIME>()) << ",";
				csv << query.syncInterval.As<int32_t>() << ",";
				csv << query.presentFlags.As<uint32_t>() << ",";
				csv << (query.allowsTearing.As<bool>() ? 1 : 0) << ",";
				csv << TranslatePresentMode(query.presentMode.As<PM_PRESENT_MODE>()) << ",";
				csv << TranslateFrameType(query.frameType.As<PM_FRAME_TYPE>()) << ",";
				WriteOptionalElement(csv, query.cpuStartTime);
				csv << ",";
				WriteOptionalElement(csv, query.msBetweenSimStart);
				csv << ",";
				WriteOptionalElement(csv, query.msBetweenPresents);
				csv << ",";
				WriteOptionalElement(csv, query.msBetweenDisplayChange);
				csv << ",";
				WriteOptionalElement(csv, query.msInPresentApi);
				csv << ",";
				WriteOptionalElement(csv, query.msRenderPresentLatency);
				csv << ",";
				WriteOptionalElement(csv, query.msUntilDisplayed);
				csv << ",";
				WriteOptionalElement(csv, query.msPcLatency);
				csv << ",";
				WriteOptionalElement(csv, query.msBetweenAppStart);
				csv << ",";
				WriteOptionalElement(csv, query.msCpuBusy);
				csv << ",";
				WriteOptionalElement(csv, query.msCpuWait);
				csv << ",";
				WriteOptionalElement(csv, query.msGpuLatency);
				csv << ",";
				WriteOptionalElement(csv, query.msGpuTime);
				csv << ",";
				WriteOptionalElement(csv, query.msGpuBusy);
				csv << ",";
				WriteOptionalElement(csv, query.msGpuWait);
				csv << ",";
				WriteOptionalDouble(csv, 0.0);
				csv << ",";
				WriteOptionalElement(csv, query.msAnimationError);
				csv << ",";
				WriteOptionalElement(csv, query.animationTime);
				csv << ",";
				WriteOptionalElement(csv, query.msFlipDelay);
				csv << ",";
				WriteOptionalElement(csv, query.msAllInputToPhotonLatency);
				csv << ",";
				WriteOptionalElement(csv, query.msClickToPhotonLatency);
				csv << ",";
				WriteOptionalElement(csv, query.msInstrumentedLatency);
				csv << "\n";
				++totalRecorded;
			});

			if (frameLimit > 0 && totalRecorded >= frameLimit) {
				break;
			}

			if (processed == 0) {
				if (totalRecorded > 0) {
					if (++emptyPollCount >= emptyLimit) {
						break;
					}
				}
				else if (elapsed >= 1.0) {
					break;
				}
			}
			else {
				emptyPollCount = 0;
			}

			if (processed == 0) {
				std::this_thread::sleep_for(8ms);
			}
		}
	}
	catch (const pmapi::ApiErrorException& e) {
		if (!opt.testExpectError) {
			throw;
		}
		errorStatus = e.GetCode();
	}

	std::string line;

	if (errorStatus) {
		std::getline(std::cin, line);
		if (line != "%err-check") {
			std::cout << "%%{err-check-error}%%" << std::endl;
			return -1;
		}
		auto&& err = pmapi::EnumMap::GetKeyMap(PM_ENUM_STATUS)->at(*errorStatus).narrowSymbol;
		std::cout << "%%{err-check-ok:" << err << "}%%" << std::endl;
	}

	while (std::getline(std::cin, line)) {
		if (line == "%quit") {
			std::cout << "%%{quit-ok}%%" << std::endl;
			std::this_thread::sleep_for(25ms);
			return 0;
		}
		else {
			std::cout << "%%{err-bad-command}%%" << std::endl;
		}
	}

	return -1;
}
