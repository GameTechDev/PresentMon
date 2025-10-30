#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
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
			if (in.etwFlushPeriodMs && *in.etwFlushPeriodMs > PM_ETW_FLUSH_PERIOD_MAX) {
				const auto sta = PM_STATUS::PM_STATUS_OUT_OF_RANGE;
				pmlog_error("Set ETW flush period failed: out of range").pmwatch(*in.etwFlushPeriodMs).code(sta);
				throw util::Except<ActionExecutionError>(sta);
			}
			stx.requestedEtwFlushPeriodMs = in.etwFlushPeriodMs;
			ctx.UpdateEtwFlushPeriod();
			if (in.etwFlushPeriodMs) {
				pmlog_dbg(std::format("Setting ETW flush period to {}ms", *in.etwFlushPeriodMs));
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
