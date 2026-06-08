#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "../ActionExecutionContext.h"
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
			std::optional<uint32_t> telemetrySamplePeriodMs;

			template<class A> void serialize(A& ar) {
				ar(telemetrySamplePeriodMs);
			}
		};
		struct Response {};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			auto telemetrySamplePeriodMs = in.telemetrySamplePeriodMs;
			if (telemetrySamplePeriodMs && *telemetrySamplePeriodMs < PM_TELEMETRY_PERIOD_MIN) {
				pmlog_warn("Telemetry period out of range; clamping to minimum")
					.pmwatch(*telemetrySamplePeriodMs).pmwatch(PM_TELEMETRY_PERIOD_MIN).diag();
				telemetrySamplePeriodMs = PM_TELEMETRY_PERIOD_MIN;
			}
			else if (telemetrySamplePeriodMs && *telemetrySamplePeriodMs > PM_TELEMETRY_PERIOD_MAX) {
				pmlog_warn("Telemetry period out of range; clamping to maximum")
					.pmwatch(*telemetrySamplePeriodMs).pmwatch(PM_TELEMETRY_PERIOD_MAX).diag();
				telemetrySamplePeriodMs = PM_TELEMETRY_PERIOD_MAX;
			}
			stx.requestedTelemetryPeriodMs = telemetrySamplePeriodMs;
			ctx.UpdateTelemetryPeriod();
			if (telemetrySamplePeriodMs) {
				pmlog_dbg(std::format("Requested telemetry sample period of {}ms by client [{}]",
					*telemetrySamplePeriodMs, stx.remotePid));
			}
			else {
				pmlog_dbg(std::format("Releasing manual telemetry period override for client [{}]", stx.remotePid));
			}
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
