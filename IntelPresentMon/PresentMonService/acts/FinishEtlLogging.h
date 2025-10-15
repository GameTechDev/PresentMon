#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include <format>

#define ACT_NAME FinishEtlLogging
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
			uint32_t etwLogSessionHandle;

			template<class A> void serialize(A& ar) {
				ar(etwLogSessionHandle);
			}
		};
		struct Response
		{
			std::string etlFilePath;

			template<class A> void serialize(A& ar) {
				ar(etlFilePath);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			if (!stx.etwLogSessionIds.contains(in.etwLogSessionHandle)) {
				pmlog_error("Client accessing etw log session without ownership");
				throw util::Except<ActionExecutionError>(PM_STATUS_SESSION_NOT_OPEN);
			}
			try {
				auto& etl = ctx.pPmon->GetEtwLogger();
				auto file = etl.FinishLogSession(in.etwLogSessionHandle);
				// move file one level higher from protected staging dir and give public ACL
				file.Ascend().MakePublic();
				stx.etwLogSessionIds.erase(in.etwLogSessionHandle);
				return Response{ .etlFilePath = file.Release().string() };
			}
			catch (...) {
				pmlog_error(ReportException("Failed to finish etw log session"));
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