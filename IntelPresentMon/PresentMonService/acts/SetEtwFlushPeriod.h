#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "../ActionExecutionContext.h"
#include <format>
#include <ranges>

#define ACT_NAME SetEtwFlushPeriod
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
			std::optional<uint32_t> etwFlushPeriodMs;

			template<class A> void serialize(A& ar) {
				ar(etwFlushPeriodMs);
			}
		};
		struct Response {};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			auto etwFlushPeriodMs = in.etwFlushPeriodMs;
			if (etwFlushPeriodMs && *etwFlushPeriodMs < PM_ETW_FLUSH_PERIOD_MIN) {
				pmlog_warn("ETW flush period out of range; clamping to minimum")
					.pmwatch(*etwFlushPeriodMs).pmwatch(PM_ETW_FLUSH_PERIOD_MIN).diag();
				etwFlushPeriodMs = PM_ETW_FLUSH_PERIOD_MIN;
			}
			else if (etwFlushPeriodMs && *etwFlushPeriodMs > PM_ETW_FLUSH_PERIOD_MAX) {
				pmlog_warn("ETW flush period out of range; clamping to maximum")
					.pmwatch(*etwFlushPeriodMs).pmwatch(PM_ETW_FLUSH_PERIOD_MAX).diag();
				etwFlushPeriodMs = PM_ETW_FLUSH_PERIOD_MAX;
			}
			stx.requestedEtwFlushPeriodMs = etwFlushPeriodMs;
			ctx.UpdateEtwFlushPeriod();
			if (etwFlushPeriodMs) {
				pmlog_dbg(std::format("Setting ETW flush period to {}ms", *etwFlushPeriodMs));
			}
			else {
				pmlog_dbg("Disabling manual ETW flush");
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
