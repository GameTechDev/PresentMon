#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include <format>
#include <ranges>

#define ACT_NAME SetTelemetryPeriod
#define ACT_EXEC_CTX ActionExecutionContext
#define ACT_NS ::pmon::svc::acts
#define ACT_TYPE AsyncActionBase_

namespace pmon::svc::acts
{
	using namespace ipc::act;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			uint32_t telemetrySamplePeriodMs;

			template<class A> void serialize(A& ar) {
				ar(telemetrySamplePeriodMs);
			}
		};
		struct Response {};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			// make sure requested period is within allowed range
			if (in.telemetrySamplePeriodMs) {
				if (in.telemetrySamplePeriodMs < PM_TELEMETRY_PERIOD_MIN ||
					in.telemetrySamplePeriodMs > PM_TELEMETRY_PERIOD_MAX) {
					const auto sta = PM_STATUS::PM_STATUS_OUT_OF_RANGE;
					pmlog_error("Set telemetry period failed").pmwatch(in.telemetrySamplePeriodMs).code(sta);
					throw util::Except<ActionExecutionError>(sta);
				}
			}
			// set request for this session
			stx.requestedTelemetryPeriodMs = in.telemetrySamplePeriodMs;
			// update the service
			ctx.UpdateTelemetryPeriod();

			pmlog_dbg(std::format("Requested telemetry sample period of {}ms by client [{}]",
				in.telemetrySamplePeriodMs, stx.remotePid));
			return {};
		}
	};

#ifdef PM_ASYNC_ACTION_REGISTRATION_
	ACTION_REG();
#endif
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
