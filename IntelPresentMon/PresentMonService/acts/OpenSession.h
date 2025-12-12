#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "../../Versioning/BuildId.h"
#include <format>

#define ACT_NAME OpenSession
#define ACT_EXEC_CTX ActionExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS ::pmon::svc::acts

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			uint32_t clientPid;
			std::string clientBuildId;
			std::string clientBuildConfig;

			template<class A> void serialize(A& ar) {
				ar(clientPid, clientBuildId, clientBuildConfig);
			}
		};
		struct Response {
			uint32_t servicePid;
			std::string serviceBuildId;
			std::string serviceBuildConfig;
			std::string shmPrefix;
			std::string shmSalt;

			template<class A> void serialize(A& ar) {
				ar(servicePid, serviceBuildId, serviceBuildConfig, shmPrefix, shmSalt);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.clientPid;
			stx.clientBuildId = in.clientBuildId;
			ctx.pSvc->SignalClientSessionOpened();
			pmlog_info(std::format("Open action for session #{} pid={}; [BID] cli={} svc={} [CFG] cli={} svc={}",
				stx.pConn->GetId(), in.clientPid, in.clientBuildId, bid::BuildIdShortHash(),
				in.clientBuildConfig, bid::BuildIdConfig()));
			return Response{
				.servicePid = GetCurrentProcessId(),
				.serviceBuildId = bid::BuildIdShortHash(),
				.serviceBuildConfig = bid::BuildIdConfig(),
				.shmPrefix = ctx.pPmon->GetBroadcaster().GetNamer().GetPrefix(),
				.shmSalt = ctx.pPmon->GetBroadcaster().GetNamer().GetSalt(),
			};
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