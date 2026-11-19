// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CommonUtilities/win/WinAPI.h"
#include "FrameMarkers.h"
#include "PresentMonProvider.h"
#include <stdexcept>

namespace pb2
{
	namespace
	{
		class ProviderSink final : public IFrameMarkerSink
		{
		public:
			explicit ProviderSink(bool emitFrameType)
				:
				emitFrameType_(emitFrameType)
			{
				provider_ = PresentMonProvider_Initialize();
				if (!provider_) {
					throw std::runtime_error("PresentMonProvider_Initialize failed");
				}
			}

			~ProviderSink() override
			{
				if (provider_) {
					PresentMonProvider_ShutDown(provider_);
					provider_ = nullptr;
				}
			}

			void Mark(FrameMarker marker, uint32_t frameId) override
			{
				if (!provider_) {
					return;
				}
				switch (marker) {
				case FrameMarker::SleepStart:
					PresentMonProvider_Application_SleepStart(provider_, frameId);
					break;
				case FrameMarker::SleepEnd:
					PresentMonProvider_Application_SleepEnd(provider_, frameId);
					break;
				case FrameMarker::SimulationStart:
					PresentMonProvider_Application_SimulationStart(provider_, frameId);
					break;
				case FrameMarker::SimulationEnd:
					PresentMonProvider_Application_SimulationEnd(provider_, frameId);
					break;
				case FrameMarker::RenderSubmitStart:
					PresentMonProvider_Application_RenderSubmitStart(provider_, frameId);
					break;
				case FrameMarker::RenderSubmitEnd:
					PresentMonProvider_Application_RenderSubmitEnd(provider_, frameId);
					break;
				case FrameMarker::PresentStart:
					if (emitFrameType_) {
						PresentMonProvider_PresentFrameType(provider_, frameId,
							PresentMonProvider_FrameType_Original, frameId);
					}
					PresentMonProvider_Application_PresentStart(provider_, frameId);
					break;
				case FrameMarker::PresentEnd:
					PresentMonProvider_Application_PresentEnd(provider_, frameId);
					break;
				}
			}

			void InputSample(uint32_t frameId, bool mouse, bool keyboard) override
			{
				if (!provider_) {
					return;
				}
				uint32_t type = PresentMonProvider_Input_NotSpecified;
				if (mouse) type |= PresentMonProvider_Input_MouseClick;
				if (keyboard) type |= PresentMonProvider_Input_KeyboardClick;
				PresentMonProvider_Application_InputSample(provider_, frameId,
					(PresentMonProvider_InputType)type);
			}

		private:
			PresentMonProvider* provider_ = nullptr;
			bool emitFrameType_ = true;
		};
	}

	std::unique_ptr<IFrameMarkerSink> CreateProviderSink(bool emitFrameType)
	{
		return std::make_unique<ProviderSink>(emitFrameType);
	}
}
