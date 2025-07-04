// Copyright (C) 2017-2024 Intel Corporation
// SPDX-License-Identifier: MIT
#define NOMINMAX
#include "../PresentMonUtils/StreamFormat.h"
#include "FrameEventQuery.h"
#include "../PresentMonAPIWrapperCommon/Introspection.h"
#include "../CommonUtilities/Memory.h"
#include "../CommonUtilities/Meta.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/Exception.h"
#include <algorithm>
#include <cstddef>
#include <limits>

using namespace pmon;
using namespace pmon::util;
using Context = PM_FRAME_QUERY::Context;

namespace pmon::mid
{
	class GatherCommand_
	{
	public:
		virtual ~GatherCommand_() = default;
		virtual void Gather(Context& ctx, uint8_t* pDestBlob) const = 0;
		virtual uint32_t GetBeginOffset() const = 0;
		virtual uint32_t GetEndOffset() const = 0;
		virtual uint32_t GetOutputOffset() const = 0;
		uint32_t GetDataSize() const { return GetEndOffset() - GetOutputOffset(); }
		uint32_t GetTotalSize() const { return GetEndOffset() - GetBeginOffset(); }
	};
}

namespace
{
	double TimestampDeltaToMilliSeconds(uint64_t timestampDelta, double performanceCounterPeriodMs)
	{
		return performanceCounterPeriodMs * double(timestampDelta);
	}

	double TimestampDeltaToUnsignedMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo, double performanceCounterPeriodMs)
	{
		return timestampFrom == 0 || timestampTo <= timestampFrom ? 0.0 : 
			TimestampDeltaToMilliSeconds(timestampTo - timestampFrom, performanceCounterPeriodMs);
	}

	double TimestampDeltaToMilliSeconds(uint64_t timestampFrom, uint64_t timestampTo, double performanceCounterPeriodMs)
	{
		return timestampFrom == 0 || timestampTo == 0 || timestampFrom == timestampTo ? 0.0 :
			timestampTo > timestampFrom ? TimestampDeltaToMilliSeconds(timestampTo - timestampFrom, performanceCounterPeriodMs)
			: -TimestampDeltaToMilliSeconds(timestampFrom - timestampTo, performanceCounterPeriodMs);
	}

	template<auto pMember>
	constexpr auto GetSubstructurePointer()
	{
		using SubstructureType = util::MemberPointerInfo<decltype(pMember)>::StructType;
		if constexpr (std::same_as<SubstructureType, PmNsmPresentEvent>) {
			return &PmNsmFrameData::present_event;
		}
		else if constexpr (std::same_as<SubstructureType, PresentMonPowerTelemetryInfo>) {
			return &PmNsmFrameData::power_telemetry;
		}
		else if constexpr (std::same_as<SubstructureType, CpuTelemetryInfo>) {
			return &PmNsmFrameData::cpu_telemetry;
		}
	}

	template<auto pMember>
	class CopyGatherCommand_ : public mid::GatherCommand_
	{
		using Type = util::MemberPointerInfo<decltype(pMember)>::MemberType;
	public:
		CopyGatherCommand_(size_t nextAvailableByteOffset, uint16_t index = 0)
			:
			inputIndex_{ index }
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(Type));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			constexpr auto pSubstruct = GetSubstructurePointer<pMember>();
			if constexpr (std::is_array_v<Type>) {
				if constexpr (std::is_same_v<std::remove_extent_t<Type>, char>) {
					const auto val = (ctx.pSourceFrameData->*pSubstruct.*pMember)[inputIndex_];
					// TODO: only getting first character of application name. Hmmm.
					strncpy_s(reinterpret_cast<char*>(&pDestBlob[outputOffset_]), 260, &val, _TRUNCATE);
				}
				else {
					const auto val = (ctx.pSourceFrameData->*pSubstruct.*pMember)[inputIndex_];
					reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
				}
			}
			else {
				const auto val = ctx.pSourceFrameData->*pSubstruct.*pMember;
				reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(Type);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
		uint16_t inputIndex_;
	};
	class CopyGatherFrameTypeCommand_ : public mid::GatherCommand_
	{
	public:
		CopyGatherFrameTypeCommand_(size_t nextAvailableByteOffset, uint16_t index = 0)
			:
			inputIndex_{ index }
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(FrameType));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
            auto val = ctx.pSourceFrameData->present_event.Displayed_FrameType[ctx.sourceFrameDisplayIndex];
			// Currently not reporting out not set or repeated frames.
			if (val == FrameType::NotSet || val == FrameType::Repeated) {
				val = FrameType::Application;
			}
            reinterpret_cast<std::remove_const_t<decltype(val)>&>(pDestBlob[outputOffset_]) = val;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(FrameType);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
		uint16_t inputIndex_;
	};
	template<uint64_t PmNsmPresentEvent::* pMember, uint64_t PmNsmPresentEvent::* pAppPropagatedMember, bool usesAppIndex>
	class QpcDurationGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		QpcDurationGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
            if constexpr (usesAppIndex) {
				if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
					if (ctx.pSourceFrameData->present_event.*pAppPropagatedMember != 0) {
						const auto val = ctx.performanceCounterPeriodMs * double(ctx.pSourceFrameData->present_event.*pAppPropagatedMember);
						reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
						return;
					}
					else if (ctx.pSourceFrameData->present_event.*pMember != 0) {
						const auto val = ctx.performanceCounterPeriodMs * double(ctx.pSourceFrameData->present_event.*pMember);
						reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
						return;
					}
				}
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			} else {
				const auto val = ctx.pSourceFrameData->present_event.*pMember != 0 ? 
					ctx.performanceCounterPeriodMs * double(ctx.pSourceFrameData->present_event.*pMember) : 0.;
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}

		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<uint64_t PmNsmPresentEvent::* pFromMember, uint64_t PmNsmPresentEvent::* pBackupFromMember, uint64_t PmNsmPresentEvent::* pToMember>
	class QpcDeltaGatherWithBackupCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		QpcDeltaGatherWithBackupCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			const auto qpcFrom = ctx.pSourceFrameData->present_event.*pFromMember;
			const auto qpcBackupFrom = ctx.pSourceFrameData->present_event.*pBackupFromMember;
            uint64_t qpcTo = 0;
			if (ctx.pSourceFrameData->present_event.AppPropagatedGPUStartTime != 0) {
                qpcTo = ctx.pSourceFrameData->present_event.AppPropagatedGPUStartTime;
            }
            else {
                qpcTo = ctx.pSourceFrameData->present_event.*pToMember;
			}
			auto instrumentedStart = qpcFrom != 0 ? qpcFrom : qpcBackupFrom;
			if (instrumentedStart != 0) {
				if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
					const auto val = TimestampDeltaToUnsignedMilliSeconds(instrumentedStart, qpcTo, ctx.performanceCounterPeriodMs);
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				}
				else {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
				}
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<uint64_t PmNsmPresentEvent::* pFromMember, uint64_t PmNsmPresentEvent::* pToMember, bool usesAppIndex>
	class QpcDeltaGatherFromToCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		QpcDeltaGatherFromToCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			const auto qpcFrom = ctx.pSourceFrameData->present_event.*pFromMember;
			const auto qpcTo = ctx.pSourceFrameData->present_event.*pToMember;
			if (qpcFrom != 0 && qpcTo != 0) {
                if constexpr (usesAppIndex) {
                    if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
                        const auto val = TimestampDeltaToUnsignedMilliSeconds(qpcFrom, qpcTo, ctx.performanceCounterPeriodMs);
                        reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
                    }
                    else {
                        reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
                            std::numeric_limits<double>::quiet_NaN();
                    }
                }
				else {
					const auto val = TimestampDeltaToUnsignedMilliSeconds(qpcFrom, qpcTo, ctx.performanceCounterPeriodMs);
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				}
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<uint64_t PmNsmPresentEvent::* pToMember>
	class QpcDeltaGatherToCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		QpcDeltaGatherToCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			const auto qpcFrom = ctx.previousPresentStartQpc;
			const auto qpcTo = ctx.pSourceFrameData->present_event.*pToMember;
			if (qpcFrom != 0 && qpcTo != 0) {
				const auto val = TimestampDeltaToUnsignedMilliSeconds(qpcFrom, qpcTo, ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			} else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class GpuTimeGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		GpuTimeGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
                double gpuDuration = 0.;
				if (ctx.pSourceFrameData->present_event.AppPropagatedGPUStartTime != 0) {
                    gpuDuration = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.AppPropagatedGPUStartTime,
                        ctx.pSourceFrameData->present_event.AppPropagatedReadyTime, ctx.performanceCounterPeriodMs);
                } else {
                    gpuDuration = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.GPUStartTime,
                        ctx.pSourceFrameData->present_event.ReadyTime, ctx.performanceCounterPeriodMs);
                }
                double gpuBusy = 0.;
                if (ctx.pSourceFrameData->present_event.AppPropagatedGPUDuration != 0) {
                    gpuBusy = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.AppPropagatedGPUDuration,
                        ctx.performanceCounterPeriodMs);
                } else {
                    gpuBusy = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.GPUDuration,
                        ctx.performanceCounterPeriodMs);
                }
				const auto gpuWait = std::max(0., gpuDuration - gpuBusy);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = gpuBusy + gpuWait;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class ClickToPhotonGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		ClickToPhotonGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			uint64_t start = ctx.pSourceFrameData->present_event.InputTime;
			if (start == 0ull) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
				auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
				if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
					ScreenTime = ii->second.displayQpc;
				}
				const auto val = TimestampDeltaToUnsignedMilliSeconds(start,
					ScreenTime, ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else
			{
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class DroppedGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DroppedGatherCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			reinterpret_cast<bool&>(pDestBlob[outputOffset_]) = ctx.dropped;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(bool);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	template<uint64_t PmNsmPresentEvent::* pEnd, bool calcPresentStartTime>
	class StartDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		StartDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if constexpr (calcPresentStartTime) {
				const auto qpcDuration = ctx.pSourceFrameData->present_event.*pEnd - ctx.qpcStart;
				const auto val = ctx.performanceCounterPeriodMs * double(qpcDuration);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			} else {
				const auto qpcDuration = ctx.cpuStart - ctx.qpcStart;
				const auto val = ctx.performanceCounterPeriodMs * double(qpcDuration);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class CpuFrameQpcGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		CpuFrameQpcGatherCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			reinterpret_cast<uint64_t&>(pDestBlob[outputOffset_]) = ctx.cpuStart;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(uint64_t);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	template<uint64_t PmNsmPresentEvent::* pEnd, uint64_t PmNsmPresentEvent::* pPropagatedEnd, bool doDroppedCheck>
	class CpuFrameQpcDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		CpuFrameQpcDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if constexpr (doDroppedCheck) {
				if (ctx.dropped) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
                if (ctx.pSourceFrameData->present_event.*pPropagatedEnd != 0) {
					const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart,
						ctx.pSourceFrameData->present_event.*pPropagatedEnd, ctx.performanceCounterPeriodMs);
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
                } else {
					const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart,
						ctx.pSourceFrameData->present_event.*pEnd, ctx.performanceCounterPeriodMs);
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				}
			}
			else{
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<uint64_t PmNsmPresentEvent::* pFrom>
	class DisplayLatencyGatherFromCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DisplayLatencyGatherFromCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}

			// Calculate the current frame's displayed time 
			auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
				ScreenTime = ii->second.displayQpc;
			}
			auto NextScreenTime = ctx.sourceFrameDisplayIndex == ctx.pSourceFrameData->present_event.DisplayedCount - 1
				? ctx.nextDisplayedQpc
				: ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex + 1];
			const auto displayedTime = TimestampDeltaToUnsignedMilliSeconds(ScreenTime, NextScreenTime, ctx.performanceCounterPeriodMs);

			uint64_t startQpc = 0;
			if (ctx.pSourceFrameData->present_event.*pFrom == 0 ||
				displayedTime == 0) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.*pFrom,
				ScreenTime,
				ctx.performanceCounterPeriodMs);
			reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			return;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<bool isXellDisplayLatency, bool isBetweenDisplayChange>
	class DisplayLatencyGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DisplayLatencyGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
            if (ctx.dropped) {
                reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
                    std::numeric_limits<double>::quiet_NaN();
                return;
            }

			// Calculate the current frame's displayed time 
			auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
				ScreenTime = ii->second.displayQpc;
			}
			auto NextScreenTime = ctx.sourceFrameDisplayIndex == ctx.pSourceFrameData->present_event.DisplayedCount - 1
				? ctx.nextDisplayedQpc
				: ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex + 1];
			const auto displayedTime = TimestampDeltaToUnsignedMilliSeconds(ScreenTime, NextScreenTime, ctx.performanceCounterPeriodMs);

			uint64_t startQpc = 0;
			if constexpr (isXellDisplayLatency) {
				if ((ctx.pSourceFrameData->present_event.AppSleepEndTime == 0 &&
					 ctx.pSourceFrameData->present_event.AppSimStartTime == 0) ||
					 displayedTime == 0) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
				const auto xellStartTime = ctx.pSourceFrameData->present_event.AppSleepEndTime != 0 ?
					ctx.pSourceFrameData->present_event.AppSleepEndTime :
					ctx.pSourceFrameData->present_event.AppSimStartTime;
				const auto val = TimestampDeltaToUnsignedMilliSeconds(xellStartTime,
					ScreenTime,
					ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				return;
            }
			else if constexpr (isBetweenDisplayChange) {
				if (ctx.previousDisplayedQpc == 0) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
				const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.previousDisplayedQpc,	ScreenTime,
					ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				return;
			} else {
				if (displayedTime == 0) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
				const auto val = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart, ScreenTime, 
					ctx.performanceCounterPeriodMs);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				return;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class ReturnNanGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		ReturnNanGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
				std::numeric_limits<double>::quiet_NaN();
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class DisplayDifferenceGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		DisplayDifferenceGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}
			auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
				ScreenTime = ii->second.displayQpc;
			}
			auto NextScreenTime = ctx.sourceFrameDisplayIndex == ctx.pSourceFrameData->present_event.DisplayedCount - 1
                ? ctx.nextDisplayedQpc
                : ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex + 1];
			const auto val = TimestampDeltaToUnsignedMilliSeconds(ScreenTime, NextScreenTime, ctx.performanceCounterPeriodMs);
			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	template<bool doDroppedCheck, bool doZeroCheck>
	class AnimationErrorGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		AnimationErrorGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t) util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if constexpr (doDroppedCheck) {
				if (ctx.dropped) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}
			if constexpr (doZeroCheck) {
				if (ctx.frameTimingData.lastDisplayedAppSimStartTime == 0 && ctx.lastDisplayedCpuStart == 0) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
				auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
				if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
					ScreenTime = ii->second.displayQpc;
				}
				auto NextScreenTime = ctx.sourceFrameDisplayIndex == ctx.pSourceFrameData->present_event.DisplayedCount - 1
					? ctx.nextDisplayedQpc
					: ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex + 1];
				const auto displayedTime = TimestampDeltaToUnsignedMilliSeconds(ScreenTime, NextScreenTime, ctx.performanceCounterPeriodMs);
				if (displayedTime == 0.0) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
				auto PrevScreenTime = ctx.frameTimingData.lastDisplayedAppScreenTime;
				// Next calculate the animation error. First calculate the simulation
				// start time. Simulation start can be either an app provided sim start time via the provider or
				// PCL stats or, if not present,the cpu start.
				uint64_t simStartTime = 0;
				if (ctx.frameTimingData.animationErrorSource == AnimationErrorSource::AppProvider && 
					ctx.frameTimingData.lastDisplayedAppSimStartTime != 0) {
					// If the app provider is the source of the animation error then use the app sim start time.
					simStartTime = ctx.pSourceFrameData->present_event.AppSimStartTime;
				}
				else if (ctx.frameTimingData.animationErrorSource == AnimationErrorSource::PCLatency &&
					ctx.frameTimingData.lastDisplayedAppSimStartTime != 0) {
					// If the pcl latency is the source of the animation error then use the pcl sim start time.
					simStartTime = ctx.pSourceFrameData->present_event.PclSimStartTime;
				}
				else if (ctx.frameTimingData.lastDisplayedAppSimStartTime == 0) {
					// If the cpu start time is the source of the animation error then use the cpu start time.
					simStartTime = ctx.cpuStart;
				}
				auto PrevSimStartTime = ctx.frameTimingData.lastDisplayedAppSimStartTime != 0 ?
					ctx.frameTimingData.lastDisplayedAppSimStartTime :
					ctx.lastDisplayedCpuStart;
				// If the simulation start time is less than the last displated simulation start time it means
				// we are transitioning to app provider events.
				if (simStartTime > PrevSimStartTime) {
					const auto val = TimestampDeltaToMilliSeconds(ScreenTime - PrevScreenTime,
						simStartTime - PrevSimStartTime, ctx.performanceCounterPeriodMs);
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
				} else {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class AnimationTimeGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		AnimationTimeGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = std::numeric_limits<double>::quiet_NaN();
				return;
			}

			// Calculate the current frame's displayed time 
			
			// Check to see if the current present is collapsed and if so use the correct display qpc.
			auto ScreenTime = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
				ScreenTime = ii->second.displayQpc;
			}
			auto NextScreenTime = ctx.sourceFrameDisplayIndex == ctx.pSourceFrameData->present_event.DisplayedCount - 1
				? ctx.nextDisplayedQpc
				: ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex + 1];
			const auto displayedTime = TimestampDeltaToUnsignedMilliSeconds(ScreenTime, NextScreenTime, ctx.performanceCounterPeriodMs);
			if (ctx.sourceFrameDisplayIndex != ctx.appIndex ||
				displayedTime == 0.0) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = std::numeric_limits<double>::quiet_NaN();
				return;
			}
			const auto firstSimStartTime = ctx.frameTimingData.firstAppSimStartTime != 0 ?
				ctx.frameTimingData.firstAppSimStartTime :
				ctx.qpcStart;
			uint64_t currentSimTime = 0;
			if (ctx.frameTimingData.animationErrorSource == AnimationErrorSource::AppProvider) {
				if (ctx.frameTimingData.lastDisplayedAppSimStartTime != 0) {
					// If the app provider is the source of the animation error then use the app sim start time.
					currentSimTime = ctx.pSourceFrameData->present_event.AppSimStartTime;
				}
				else {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
					return;
				}
			}
			else if (ctx.frameTimingData.animationErrorSource == AnimationErrorSource::PCLatency) {
				if (ctx.frameTimingData.lastDisplayedAppSimStartTime != 0) {
					// If the pcl latency is the source of the animation error then use the pcl sim start time.
					currentSimTime = ctx.pSourceFrameData->present_event.PclSimStartTime;
				}
				else {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
					return;
				}
			}
			else if (ctx.frameTimingData.lastDisplayedAppSimStartTime == 0) {
				// If the cpu start time is the source of the animation error then use the cpu start time.
				currentSimTime = ctx.cpuStart;
			}
			const auto val = TimestampDeltaToUnsignedMilliSeconds(firstSimStartTime, currentSimTime, ctx.performanceCounterPeriodMs);
			reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class CpuFrameQpcFrameTimeCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		CpuFrameQpcFrameTimeCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
                double cpuBusy = 0.;
				if (ctx.pSourceFrameData->present_event.AppPropagatedPresentStartTime != 0) {
                    cpuBusy = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart, ctx.pSourceFrameData->present_event.AppPropagatedPresentStartTime,
						ctx.performanceCounterPeriodMs);
                }
                else {
                    cpuBusy = TimestampDeltaToUnsignedMilliSeconds(ctx.cpuStart, ctx.pSourceFrameData->present_event.PresentStartTime,
                        ctx.performanceCounterPeriodMs);
				}
                double cpuWait = 0.;
				if (ctx.pSourceFrameData->present_event.AppPropagatedTimeInPresent != 0) {
                    cpuWait = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.AppPropagatedTimeInPresent,
                        ctx.performanceCounterPeriodMs);
                }
                else {
                    cpuWait = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.TimeInPresent,
                        ctx.performanceCounterPeriodMs);
                }
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = cpuBusy + cpuWait;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(uint64_t);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	class GpuWaitGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		GpuWaitGatherCommand_(size_t nextAvailableByteOffset) : outputOffset_{ (uint32_t)nextAvailableByteOffset } {}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
                double gpuDuration = 0.;
                double gpuBusy = 0.;
                if (ctx.pSourceFrameData->present_event.AppPropagatedGPUStartTime != 0) {
                    gpuDuration = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.AppPropagatedGPUStartTime,
                        ctx.pSourceFrameData->present_event.AppPropagatedReadyTime, ctx.performanceCounterPeriodMs);
					gpuBusy = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.AppPropagatedGPUDuration,
						ctx.performanceCounterPeriodMs);
                }
                else {
                    gpuDuration = TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.GPUStartTime,
                        ctx.pSourceFrameData->present_event.ReadyTime, ctx.performanceCounterPeriodMs);
					gpuBusy = TimestampDeltaToMilliSeconds(ctx.pSourceFrameData->present_event.GPUDuration,
						ctx.performanceCounterPeriodMs);
                }
				const auto val = std::max(0., gpuDuration - gpuBusy);
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = 0.;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(uint64_t);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
	};
	template<uint64_t PmNsmPresentEvent::* pStart, bool doDroppedCheck, bool isMouseClick>
	class InputLatencyGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		InputLatencyGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if constexpr (doDroppedCheck) {
				if (ctx.dropped) {
					reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
						std::numeric_limits<double>::quiet_NaN();
					return;
				}
			}

			// Check to see if the current present is collapsed and if so use the correct display qpc.
			uint64_t displayQpc = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
				displayQpc = ii->second.displayQpc;
			}

			double updatedInputTime = 0.;
			double val = 0.;
			if (ctx.sourceFrameDisplayIndex == ctx.appIndex) {
				if (isMouseClick) {
					updatedInputTime = ctx.lastReceivedNotDisplayedClickQpc == 0 ? 0. :
						TimestampDeltaToUnsignedMilliSeconds(ctx.lastReceivedNotDisplayedClickQpc,
							displayQpc,
							ctx.performanceCounterPeriodMs);
					val = ctx.pSourceFrameData->present_event.*pStart == 0 ? updatedInputTime :
						TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.*pStart,
							displayQpc,
							ctx.performanceCounterPeriodMs);
					ctx.lastReceivedNotDisplayedClickQpc = 0;
				}
				else {
					updatedInputTime = ctx.lastReceivedNotDisplayedAllInputTime == 0 ? 0. :
						TimestampDeltaToUnsignedMilliSeconds(ctx.lastReceivedNotDisplayedAllInputTime,
							displayQpc,
							ctx.performanceCounterPeriodMs);
					val = ctx.pSourceFrameData->present_event.*pStart == 0 ? updatedInputTime :
						TimestampDeltaToUnsignedMilliSeconds(ctx.pSourceFrameData->present_event.*pStart,
							displayQpc,
							ctx.performanceCounterPeriodMs);
					ctx.lastReceivedNotDisplayedAllInputTime = 0;
				}
			}

			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class PcLatencyGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		PcLatencyGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}

			double val = 0.;
			auto simStartTime = ctx.pSourceFrameData->present_event.PclSimStartTime != 0 ?
				ctx.pSourceFrameData->present_event.PclSimStartTime : 
				ctx.frameTimingData.lastAppSimStartTime;
			if (ctx.avgInput2Fs == 0. || simStartTime == 0) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}

			// Check to see if the current present is collapsed and if so use the correct display qpc.
			uint64_t displayQpc = ctx.pSourceFrameData->present_event.Displayed_ScreenTime[ctx.sourceFrameDisplayIndex];
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
				displayQpc = ii->second.displayQpc;
			}

			val = ctx.avgInput2Fs +
				TimestampDeltaToUnsignedMilliSeconds(simStartTime, 
					displayQpc,
					ctx.performanceCounterPeriodMs);

			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class BetweenSimStartsGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		BetweenSimStartsGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{
			double val = 0.;
			uint64_t currentSimStartTime = 0;
			if (ctx.pSourceFrameData->present_event.PclSimStartTime != 0) {
				currentSimStartTime = ctx.pSourceFrameData->present_event.PclSimStartTime;
            } else if (ctx.pSourceFrameData->present_event.AppSimStartTime != 0) {
				currentSimStartTime = ctx.pSourceFrameData->present_event.AppSimStartTime;
            }

			if (ctx.frameTimingData.lastAppSimStartTime == 0 || currentSimStartTime == 0) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}

			val = TimestampDeltaToUnsignedMilliSeconds(
				ctx.frameTimingData.lastAppSimStartTime, currentSimStartTime,
				ctx.performanceCounterPeriodMs);

			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
	class FlipDelayGatherCommand_ : public pmon::mid::GatherCommand_
	{
	public:
		FlipDelayGatherCommand_(size_t nextAvailableByteOffset)
		{
			outputPaddingSize_ = (uint16_t)util::GetPadding(nextAvailableByteOffset, alignof(double));
			outputOffset_ = uint32_t(nextAvailableByteOffset) + outputPaddingSize_;
		}
		void Gather(Context& ctx, uint8_t* pDestBlob) const override
		{

			if (ctx.dropped) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
				return;
			}

            // Calculate the current frame's flip delay
			uint64_t flipDelay = ctx.pSourceFrameData->present_event.FlipDelay;
			auto ii = ctx.frameTimingData.flipDelayDataMap.find(ctx.pSourceFrameData->present_event.FrameId);
			if (ii != ctx.frameTimingData.flipDelayDataMap.end()) {
                flipDelay = ii->second.flipDelay;
			}

			double val = 0.;
			val = TimestampDeltaToMilliSeconds(flipDelay, ctx.performanceCounterPeriodMs);

			if (val == 0.) {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) =
					std::numeric_limits<double>::quiet_NaN();
			}
			else {
				reinterpret_cast<double&>(pDestBlob[outputOffset_]) = val;
			}
		}
		uint32_t GetBeginOffset() const override
		{
			return outputOffset_ - outputPaddingSize_;
		}
		uint32_t GetEndOffset() const override
		{
			return outputOffset_ + alignof(double);
		}
		uint32_t GetOutputOffset() const override
		{
			return outputOffset_;
		}
	private:
		uint32_t outputOffset_;
		uint16_t outputPaddingSize_;
	};
}

PM_FRAME_QUERY::PM_FRAME_QUERY(std::span<PM_QUERY_ELEMENT> queryElements)
{
	// TODO: validation
	//	only allow array index zero if not array type in nsm
	//	fail if array index out of bounds
	//  fail if any metrics aren't event-compatible
	//  fail if any stats other than NONE are specified
	//  fail if intro size doesn't match nsm size
	
	// we need to keep track of how many non-universal devices are specified
	// current release: only 1 gpu device maybe be polled at a time
	for (auto& q : queryElements) {
		// validate that maximum 1 device (gpu) id is specified throughout the query
		if (q.deviceId != 0) {
			if (!referencedDevice_) {
				referencedDevice_ = q.deviceId;
			}
			else if (*referencedDevice_ != q.deviceId) {
				pmlog_error("Cannot specify 2 different non-universal devices in the same query")
					.pmwatch(*referencedDevice_).pmwatch(q.deviceId).diag();
				throw Except<Exception>("2 different non-universal devices in same query");
			}
		}
		if (auto pElement = MapQueryElementToGatherCommand_(q, blobSize_)) {
			gatherCommands_.push_back(std::move(pElement));
			const auto& cmd = gatherCommands_.back();
			q.dataSize = cmd->GetDataSize();
			q.dataOffset = cmd->GetOutputOffset();
			blobSize_ += cmd->GetTotalSize();
		}
	}
	// make sure blobs are a multiple of 16 so that blobs in array always start 16-aligned
	blobSize_ += util::GetPadding(blobSize_, 16);
}

PM_FRAME_QUERY::~PM_FRAME_QUERY() = default;

void PM_FRAME_QUERY::GatherToBlob(Context& ctx, uint8_t* pDestBlob) const
{
	for (auto& cmd : gatherCommands_) {
		cmd->Gather(ctx, pDestBlob);
	}
}

size_t PM_FRAME_QUERY::GetBlobSize() const
{
	return blobSize_;
}

std::optional<uint32_t> PM_FRAME_QUERY::GetReferencedDevice() const
{
	return referencedDevice_;
}

std::unique_ptr<mid::GatherCommand_> PM_FRAME_QUERY::MapQueryElementToGatherCommand_(const PM_QUERY_ELEMENT& q, size_t pos)
{
	using Pre = PmNsmPresentEvent;
	using Gpu = PresentMonPowerTelemetryInfo;
	using Cpu = CpuTelemetryInfo;

	switch (q.metric) {
	// temporary static metric lookup via nsm
	// only implementing the ones used by appcef right now... others available in the future
	// TODO: implement fill for all static OR drop support for filling static
	case PM_METRIC_APPLICATION:
		return std::make_unique<CopyGatherCommand_<&Pre::application>>(pos);
	case PM_METRIC_GPU_MEM_SIZE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_total_size_b>>(pos);
	case PM_METRIC_GPU_MEM_MAX_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_max_bandwidth_bps>>(pos);

	case PM_METRIC_SWAP_CHAIN_ADDRESS:
		return std::make_unique<CopyGatherCommand_<&Pre::SwapChainAddress>>(pos);
	case PM_METRIC_GPU_BUSY:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::GPUDuration, &Pre::AppPropagatedGPUDuration, 1>>(pos);
	case PM_METRIC_DROPPED_FRAMES:
		return std::make_unique<DroppedGatherCommand_>(pos);
	case PM_METRIC_PRESENT_MODE:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentMode>>(pos);
	case PM_METRIC_PRESENT_RUNTIME:
		return std::make_unique<CopyGatherCommand_<&Pre::Runtime>>(pos);
	case PM_METRIC_CPU_START_QPC:
		return std::make_unique<CpuFrameQpcGatherCommand_>(pos);
	case PM_METRIC_ALLOWS_TEARING:
		return std::make_unique<CopyGatherCommand_<&Pre::SupportsTearing>>(pos);
	case PM_METRIC_FRAME_TYPE:
		return std::make_unique<CopyGatherFrameTypeCommand_>(pos);
	case PM_METRIC_SYNC_INTERVAL:
		return std::make_unique<CopyGatherCommand_<&Pre::SyncInterval>>(pos);

	case PM_METRIC_GPU_POWER:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_power_w>>(pos);
	case PM_METRIC_GPU_VOLTAGE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_voltage_v>>(pos);
	case PM_METRIC_GPU_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_frequency_mhz>>(pos);
	case PM_METRIC_GPU_TEMPERATURE:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_temperature_c>>(pos);
	case PM_METRIC_GPU_FAN_SPEED:
		return std::make_unique<CopyGatherCommand_<&Gpu::fan_speed_rpm>>(pos, q.arrayIndex);
	case PM_METRIC_GPU_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_utilization>>(pos);
	case PM_METRIC_GPU_RENDER_COMPUTE_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_render_compute_utilization>>(pos);
	case PM_METRIC_GPU_MEDIA_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_media_utilization>>(pos);
	case PM_METRIC_GPU_MEM_POWER:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_power_w>>(pos);
	case PM_METRIC_GPU_MEM_VOLTAGE:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_voltage_v>>(pos);
	case PM_METRIC_GPU_MEM_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_frequency_mhz>>(pos);
	case PM_METRIC_GPU_MEM_EFFECTIVE_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_effective_frequency_gbps>>(pos);
	case PM_METRIC_GPU_MEM_TEMPERATURE:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_temperature_c>>(pos);
	case PM_METRIC_GPU_MEM_USED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_used_b>>(pos);
	case PM_METRIC_GPU_MEM_WRITE_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_write_bandwidth_bps>>(pos);
	case PM_METRIC_GPU_MEM_READ_BANDWIDTH:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_mem_read_bandwidth_bps>>(pos);
	case PM_METRIC_GPU_POWER_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_power_limited>>(pos);
	case PM_METRIC_GPU_TEMPERATURE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_temperature_limited>>(pos);
	case PM_METRIC_GPU_CURRENT_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_current_limited>>(pos);
	case PM_METRIC_GPU_VOLTAGE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_voltage_limited>>(pos);
	case PM_METRIC_GPU_UTILIZATION_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::gpu_utilization_limited>>(pos);
	case PM_METRIC_GPU_MEM_POWER_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_power_limited>>(pos);
	case PM_METRIC_GPU_MEM_TEMPERATURE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_temperature_limited>>(pos);
	case PM_METRIC_GPU_MEM_CURRENT_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_current_limited>>(pos);
	case PM_METRIC_GPU_MEM_VOLTAGE_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_voltage_limited>>(pos);
	case PM_METRIC_GPU_MEM_UTILIZATION_LIMITED:
		return std::make_unique<CopyGatherCommand_<&Gpu::vram_utilization_limited>>(pos);

	case PM_METRIC_CPU_UTILIZATION:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_utilization>>(pos);
	case PM_METRIC_CPU_POWER:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_power_w>>(pos);
	case PM_METRIC_CPU_TEMPERATURE:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_temperature>>(pos);
	case PM_METRIC_CPU_FREQUENCY:
		return std::make_unique<CopyGatherCommand_<&Cpu::cpu_frequency>>(pos);

	case PM_METRIC_PRESENT_FLAGS:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentFlags>>(pos);
	case PM_METRIC_CPU_START_TIME:
		return std::make_unique<StartDifferenceGatherCommand_<&Pre::PresentStartTime, 0>>(pos);
	case PM_METRIC_CPU_FRAME_TIME:
	case PM_METRIC_BETWEEN_APP_START:
		return std::make_unique<CpuFrameQpcFrameTimeCommand_>(pos);
	case PM_METRIC_CPU_BUSY:
		return std::make_unique<CpuFrameQpcDifferenceGatherCommand_<&Pre::PresentStartTime, &Pre::AppPropagatedPresentStartTime, 0>>(pos);
	case PM_METRIC_CPU_WAIT:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::TimeInPresent, &Pre::AppPropagatedTimeInPresent, 1>>(pos);
	case PM_METRIC_GPU_TIME:
		return std::make_unique<GpuTimeGatherCommand_>(pos);
	case PM_METRIC_GPU_WAIT:
		return std::make_unique<GpuWaitGatherCommand_>(pos);
	case PM_METRIC_DISPLAYED_TIME:
		return std::make_unique<DisplayDifferenceGatherCommand_>(pos);
	case PM_METRIC_ANIMATION_ERROR:
		return std::make_unique<AnimationErrorGatherCommand_<1,1>>(pos);
	case PM_METRIC_ANIMATION_TIME:
		return std::make_unique<AnimationTimeGatherCommand_>(pos);
	case PM_METRIC_GPU_LATENCY:
		return std::make_unique<CpuFrameQpcDifferenceGatherCommand_<&Pre::GPUStartTime, &Pre::AppPropagatedGPUStartTime, 0>>(pos);
	case PM_METRIC_DISPLAY_LATENCY:
		return std::make_unique<DisplayLatencyGatherCommand_<0,0>>(pos);
	case PM_METRIC_CLICK_TO_PHOTON_LATENCY:
		return std::make_unique<InputLatencyGatherCommand_<&Pre::MouseClickTime, 1, 1>>(pos);
	case PM_METRIC_ALL_INPUT_TO_PHOTON_LATENCY:
		return std::make_unique<InputLatencyGatherCommand_<&Pre::InputTime, 1, 0>>(pos);
	case PM_METRIC_INSTRUMENTED_LATENCY:
		return std::make_unique<DisplayLatencyGatherCommand_<1,0>>(pos);
	case PM_METRIC_PRESENT_START_TIME:
		return std::make_unique<StartDifferenceGatherCommand_<&Pre::PresentStartTime, 1>>(pos);
	case PM_METRIC_PRESENT_START_QPC:
		return std::make_unique<CopyGatherCommand_<&Pre::PresentStartTime>>(pos);
    case PM_METRIC_IN_PRESENT_API:
		return std::make_unique<QpcDurationGatherCommand_<&Pre::TimeInPresent, &Pre::TimeInPresent, 0>>(pos);
    case PM_METRIC_UNTIL_DISPLAYED:
        return std::make_unique<DisplayLatencyGatherFromCommand_<&Pre::PresentStartTime>>(pos);
    case PM_METRIC_BETWEEN_DISPLAY_CHANGE:
        return std::make_unique<DisplayLatencyGatherCommand_<0,1>>(pos);
	case PM_METRIC_BETWEEN_PRESENTS:
        return std::make_unique<QpcDeltaGatherToCommand_<&Pre::PresentStartTime>>(pos);
	case PM_METRIC_RENDER_PRESENT_LATENCY:
		return std::make_unique<QpcDeltaGatherFromToCommand_<&Pre::PresentStartTime, &Pre::ReadyTime, 0>>(pos);
	case PM_METRIC_BETWEEN_SIMULATION_START:
        return std::make_unique<BetweenSimStartsGatherCommand_>(pos);
	case PM_METRIC_PC_LATENCY:
        return std::make_unique<PcLatencyGatherCommand_>(pos);
	case PM_METRIC_FLIP_DELAY:
		return std::make_unique<FlipDelayGatherCommand_>(pos);
	default:
		pmlog_error("unknown metric id").pmwatch((int)q.metric).diag();
		return {};
	}
}

void PM_FRAME_QUERY::Context::UpdateSourceData(const PmNsmFrameData* pSourceFrameData_in,
											   const PmNsmFrameData* pFrameDataOfNextDisplayed,
										       const PmNsmFrameData* pFrameDataOfLastPresented,
											   const PmNsmFrameData* pFrameDataOfLastAppPresented,
										       const PmNsmFrameData* pFrameDataOfLastDisplayed,
											   const PmNsmFrameData* pFrameDataOfLastAppDisplayed,	
										       const PmNsmFrameData* pFrameDataOfPreviousAppFrameOfLastAppDisplayed)
{
	pSourceFrameData = pSourceFrameData_in;
    sourceFrameDisplayIndex = 0;
	dropped = pSourceFrameData->present_event.FinalState != PresentResult::Presented || pSourceFrameData->present_event.DisplayedCount == 0;
	if (dropped) {
		if (pSourceFrameData->present_event.MouseClickTime != 0) {
			lastReceivedNotDisplayedClickQpc = pSourceFrameData->present_event.MouseClickTime;
		}
		if (pSourceFrameData->present_event.InputTime != 0) {
			lastReceivedNotDisplayedAllInputTime = pSourceFrameData->present_event.InputTime;
		}
		if (pSourceFrameData->present_event.PclSimStartTime != 0) {
			if (pSourceFrameData->present_event.PclInputPingTime != 0) {
				// This frame was dropped but we have valid pc latency input and simulation start
				// times. Calculate the initial input to sim start time.
				mAccumulatedInput2FrameStartTime =
					TimestampDeltaToUnsignedMilliSeconds(
						pSourceFrameData->present_event.PclInputPingTime,
						pSourceFrameData->present_event.PclSimStartTime,
						performanceCounterPeriodMs);
				mLastReceivedNotDisplayedPclInputTime = pSourceFrameData->present_event.PclInputPingTime;
			}
			else if (mAccumulatedInput2FrameStartTime != 0.f) {
				// This frame was also dropped and there is no pc latency input time. However, since we have
				// accumulated time this means we have a pending input that has had multiple dropped frames
				// and has not yet hit the screen. Calculate the time between the last not displayed sim start and
				// this sim start and add it to our accumulated total
				mAccumulatedInput2FrameStartTime +=
					TimestampDeltaToUnsignedMilliSeconds(
						mLastReceivedNotDisplayedPclSimStart,
						pSourceFrameData->present_event.PclSimStartTime,
						performanceCounterPeriodMs);
			}
			mLastReceivedNotDisplayedPclSimStart = pSourceFrameData->present_event.PclSimStartTime;
		}
	}

	AnimationErrorSource initAmimationErrorSource = frameTimingData.animationErrorSource;
	if (frameTimingData.firstAppSimStartTime == 0) {
		// Handle this rare case at the start of consuming frames and the application is
		// producing app frame data:
		// [0] Frame - Not Dropped           - Valid App Data -> Start of raw frame data
		// [1] Frame - Dropped | Not Dropped - Valid App Data
		// [2] Frame - Dropped | Not Dropped - Valid App Data
		// When we start processing the NSM at [0] we will not have a last presented frame data
		// and therefore not enter this function to update source the data. However the correct
		// app sim start time is at [0] regardless if [1] or [2] are dropped or not
		if (pFrameDataOfLastDisplayed) {
			if (pFrameDataOfLastDisplayed->present_event.AppSimStartTime != 0) {
				frameTimingData.firstAppSimStartTime = pFrameDataOfLastDisplayed->present_event.AppSimStartTime;
				frameTimingData.animationErrorSource = AnimationErrorSource::AppProvider;
			} else if (pFrameDataOfLastDisplayed->present_event.PclSimStartTime != 0){
				frameTimingData.firstAppSimStartTime = pFrameDataOfLastDisplayed->present_event.PclSimStartTime;
				frameTimingData.animationErrorSource = AnimationErrorSource::PCLatency;
			}
		}
		// In the case where [0] does not set the firstAppSimStartTime and the current frame is
		// either [1] or [2] and is not dropped set the firstAppSimStartTime
		if (frameTimingData.firstAppSimStartTime == 0 && !dropped) {
			if (pSourceFrameData->present_event.AppSimStartTime != 0) {
				frameTimingData.firstAppSimStartTime = pSourceFrameData->present_event.AppSimStartTime;
				frameTimingData.animationErrorSource = AnimationErrorSource::AppProvider;
			}
			else if (pSourceFrameData->present_event.PclSimStartTime != 0) {
				frameTimingData.firstAppSimStartTime = pSourceFrameData->present_event.PclSimStartTime;
				frameTimingData.animationErrorSource = AnimationErrorSource::PCLatency;
			}
		}
	}

	if (pFrameDataOfLastPresented) {
        previousPresentStartQpc = pFrameDataOfLastPresented->present_event.PresentStartTime;
	} else {
		// TODO: log issue or invalidate related columns or drop frame (or some combination)
		pmlog_info("null pFrameDataOfLastPresented");
		previousPresentStartQpc = 0;
	}

	if (pFrameDataOfLastAppPresented) {
        if (pFrameDataOfLastAppPresented->present_event.AppPropagatedPresentStartTime != 0) {
			cpuStart = pFrameDataOfLastAppPresented->present_event.AppPropagatedPresentStartTime +
				pFrameDataOfLastAppPresented->present_event.AppPropagatedTimeInPresent;
        } else {
			cpuStart = pFrameDataOfLastAppPresented->present_event.PresentStartTime +
				pFrameDataOfLastAppPresented->present_event.TimeInPresent;
		}
		if (pFrameDataOfLastPresented->present_event.PclSimStartTime != 0) {
			frameTimingData.lastAppSimStartTime = pFrameDataOfLastAppPresented->present_event.PclSimStartTime;
		} else if (pFrameDataOfLastPresented->present_event.AppSimStartTime != 0) {
			frameTimingData.lastAppSimStartTime = pFrameDataOfLastAppPresented->present_event.AppSimStartTime;
		}
	}
	else {
		// TODO: log issue or invalidate related columns or drop frame (or some combination)
		pmlog_info("null pFrameDataOfLastAppPresented");
		cpuStart = 0;
	}

	if (pFrameDataOfNextDisplayed) {
		if (!dropped) {
			uint64_t flipDelay = 0;
			uint64_t currentDisplayQpc = 0;
			uint64_t nextDisplayQpc = 0;
			// Check to see if the current frame had it's flip delay and display qpc set due to a 
            // collapsed present. If so use those values to calculate the next displayed qpc.
			auto ii = frameTimingData.flipDelayDataMap.find(pSourceFrameData->present_event.FrameId);
			if (ii != frameTimingData.flipDelayDataMap.end()) {
				flipDelay = ii->second.flipDelay;
                currentDisplayQpc = ii->second.displayQpc;
			} else {
				flipDelay = pSourceFrameData->present_event.FlipDelay;
                currentDisplayQpc = pSourceFrameData->present_event.Displayed_ScreenTime[0];
			}
			if (flipDelay != 0 && (currentDisplayQpc > pFrameDataOfNextDisplayed->present_event.Displayed_ScreenTime[0])) {
                // The next displayed frame is a collapsed present. Update the flip delay data map with the
                // current frame's flip delay and display qpc.
                FlipDelayData flipDelayData {};
                flipDelayData.flipDelay = pFrameDataOfNextDisplayed->present_event.FlipDelay + 
					(currentDisplayQpc - pFrameDataOfNextDisplayed->present_event.Displayed_ScreenTime[0]);
				flipDelayData.displayQpc = pSourceFrameData->present_event.Displayed_ScreenTime[0];
				nextDisplayedQpc = currentDisplayQpc;
				frameTimingData.flipDelayDataMap[pFrameDataOfNextDisplayed->present_event.FrameId] = flipDelayData;
			} else {
				nextDisplayedQpc = pFrameDataOfNextDisplayed->present_event.Displayed_ScreenTime[0];
			}
		} else {
			nextDisplayedQpc = pFrameDataOfNextDisplayed->present_event.Displayed_ScreenTime[0];
		}
	}
	else {
		// TODO: log issue or invalidate related columns or drop frame (or some combination)
		pmlog_info("null pFrameDataOfNextDisplayed");
		nextDisplayedQpc = 0;
	}
	
	if (pFrameDataOfLastDisplayed && pFrameDataOfLastDisplayed->present_event.DisplayedCount > 0) {
		// Check to see if the current frame had it's flip delay and display qpc set due to a 
		// collapsed present. If so use those values to calculate the next displayed qpc.
		auto ii = frameTimingData.flipDelayDataMap.find(pFrameDataOfLastDisplayed->present_event.FrameId);
		if (ii != frameTimingData.flipDelayDataMap.end()) {
			previousDisplayedQpc = ii->second.displayQpc;
		}
		else {
			previousDisplayedQpc = pFrameDataOfLastDisplayed->present_event.Displayed_ScreenTime[pFrameDataOfLastDisplayed->present_event.DisplayedCount - 1];
		}
        // Save off the frame id of the last displayed frame to allow for deletion of old flip delay data
        frameTimingData.lastDisplayedFrameId = pFrameDataOfLastDisplayed->present_event.FrameId;
	} else {
		// TODO: log issue or invalidate related columns or drop frame (or some combination)
		pmlog_info("null pFrameDataOfLastDisplayed");
		previousDisplayedQpc = 0;
	}

	if (pFrameDataOfLastAppDisplayed && pFrameDataOfLastAppDisplayed->present_event.DisplayedCount > 0) {
		// Use the animation error source to set the various last displayed sim start and app screen times
		uint64_t displayQpc = 0;
		// Check to see if the current frame had it's flip delay and display qpc set due to a 
		// collapsed present. If so use those values to calculate the next displayed qpc.
		auto ii = frameTimingData.flipDelayDataMap.find(pFrameDataOfLastAppDisplayed->present_event.FrameId);
		if (ii != frameTimingData.flipDelayDataMap.end()) {
			displayQpc = ii->second.displayQpc;
		}
		else {
			displayQpc = pFrameDataOfLastAppDisplayed->present_event.Displayed_ScreenTime[pFrameDataOfLastAppDisplayed->present_event.DisplayedCount - 1];
		}
		if (frameTimingData.animationErrorSource == AnimationErrorSource::PCLatency) {
			// Special handling is required for application data provided by PCL events. In the PCL events
			// case we use a non-zero PclSimStartTime as an indicator for an application generated frame.
			if (pFrameDataOfLastAppDisplayed->present_event.PclSimStartTime != 0) {
				frameTimingData.lastDisplayedAppSimStartTime = pFrameDataOfLastAppDisplayed->present_event.PclSimStartTime;

				frameTimingData.lastDisplayedAppScreenTime = displayQpc;
			} else {
				// If we are transistioning from CPU Start time based animation error source to PCL then we need to update the last displayed app screen time
				// for the animation error.
				if (initAmimationErrorSource != frameTimingData.animationErrorSource) {
					frameTimingData.lastDisplayedAppScreenTime = displayQpc;
				}
			}
		} else if (frameTimingData.animationErrorSource == AnimationErrorSource::AppProvider) {
            // The above transition check in the PCL case is not needed for the AppProvider case as with the app provided
			// we know which frames are applicatoin generated.
			frameTimingData.lastDisplayedAppSimStartTime = pFrameDataOfLastAppDisplayed->present_event.AppSimStartTime;
			frameTimingData.lastDisplayedAppScreenTime = displayQpc;
		} else {
			frameTimingData.lastDisplayedAppScreenTime = displayQpc;
		}
	}
	else {
		// TODO: log issue or invalidate related columns or drop frame (or some combination)
		pmlog_info("null pFrameDataOfLastAppDisplayed");
	}

	if (pFrameDataOfPreviousAppFrameOfLastAppDisplayed) {
		if (pFrameDataOfPreviousAppFrameOfLastAppDisplayed->present_event.AppPropagatedPresentStartTime != 0) {
            lastDisplayedCpuStart = pFrameDataOfPreviousAppFrameOfLastAppDisplayed->present_event.AppPropagatedPresentStartTime +
				pFrameDataOfPreviousAppFrameOfLastAppDisplayed->present_event.AppPropagatedTimeInPresent;
        } else {
			lastDisplayedCpuStart = pFrameDataOfPreviousAppFrameOfLastAppDisplayed->present_event.PresentStartTime +
				pFrameDataOfPreviousAppFrameOfLastAppDisplayed->present_event.TimeInPresent;
		}
	}
	else {
		pmlog_dbg("null pPreviousFrameDataOfLastDisplayed");
		lastDisplayedCpuStart = 0;
	}
	appIndex = std::numeric_limits<size_t>::max();
    if (pSourceFrameData->present_event.DisplayedCount > 0) {
		for (size_t i = 0; i < pSourceFrameData->present_event.DisplayedCount; ++i) {
			if (pSourceFrameData->present_event.Displayed_FrameType[i] == FrameType::NotSet ||
				pSourceFrameData->present_event.Displayed_FrameType[i] == FrameType::Application) {
				appIndex = i;
				break;
			}
		}
	} else {
        appIndex = 0;
	}

	return;
}
