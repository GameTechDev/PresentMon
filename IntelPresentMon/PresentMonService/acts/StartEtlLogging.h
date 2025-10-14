#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include <format>

#define ACT_NAME StartEtlLogging
#define ACT_EXEC_CTX ActionExecutionContext
#define ACT_NS ::pmon::svc::acts
#define ACT_TYPE AsyncActionBase_

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			template<class A> void serialize(A& ar) {
			}
		};
		struct Response
		{
			uint32_t etwLogSessionHandle;
			template<class A> void serialize(A& ar) {
				ar(etwLogSessionHandle);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			try {
				auto& etl = ctx.pPmon->GetEtwLogger();
				const Response out{ .etwLogSessionHandle = etl.StartLogSession() };
				stx.etwLogSessionIds.insert(out.etwLogSessionHandle);
				return out;
			}
			catch (...) {
				pmlog_error(ReportException("Failed to start etw log session"));
				throw util::Except<ActionExecutionError>(PM_STATUS_FAILURE);
			}
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