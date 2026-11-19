// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CommonUtilities/win/WinAPI.h"
#include "FrameMarkers.h"
#include <TraceLoggingProvider.h>
#include <format>
#include <stdexcept>

TRACELOGGING_DEFINE_PROVIDER(
	g_pb2PclProvider,
	"PCLStats",
	(0x0D216F06, 0x82A6, 0x4D49, 0xBC, 0x4F, 0x8F, 0x38, 0xAE, 0x56, 0xEF, 0xAB));

namespace pb2
{
	namespace
	{
		class PclStatsSink final : public IFrameMarkerSink
		{
		public:
			PclStatsSink()
			{
				const ULONG status = TraceLoggingRegister(g_pb2PclProvider);
				if (status != ERROR_SUCCESS) {
					throw std::runtime_error(std::format(
						"TraceLoggingRegister(PCL) failed: {}", status));
				}
				registered_ = true;
				TraceLoggingWrite(g_pb2PclProvider, "PCLStatsInit");
			}

			~PclStatsSink() override
			{
				if (registered_) {
					TraceLoggingWrite(g_pb2PclProvider, "PCLStatsShutdown");
					TraceLoggingUnregister(g_pb2PclProvider);
					registered_ = false;
				}
			}

			void Mark(FrameMarker marker, uint32_t frameId) override
			{
				uint32_t pclMarker = 0;
				switch (marker) {
				case FrameMarker::SimulationStart:
					pclMarker = 0;
					break;
				case FrameMarker::SimulationEnd: pclMarker = 1; break;
				case FrameMarker::RenderSubmitStart: pclMarker = 2; break;
				case FrameMarker::RenderSubmitEnd: pclMarker = 3; break;
				case FrameMarker::PresentStart: pclMarker = 4; break;
				case FrameMarker::PresentEnd: pclMarker = 5; break;
				default: return;
				}
				WriteMarker_(pclMarker, frameId);
				if (marker == FrameMarker::SimulationStart) {
					// Latency ping at simulation start (matches prior PCL adapter timing).
					TraceLoggingWrite(g_pb2PclProvider, "PCLStatsInput");
					WriteMarker_(8u, frameId);
				}
			}

			void WriteMarker_(uint32_t marker, uint32_t frameId) const
			{
				const uint64_t frame64 = (uint64_t)frameId;
				TraceLoggingWrite(
					g_pb2PclProvider,
					"PCLStatsEvent",
					TraceLoggingUInt32(marker, "Marker"),
					TraceLoggingUInt64(frame64, "FrameID"));
			}

		private:
			bool registered_ = false;
		};
	}

	std::unique_ptr<IFrameMarkerSink> CreatePclStatsSink()
	{
		return std::make_unique<PclStatsSink>();
	}
}
