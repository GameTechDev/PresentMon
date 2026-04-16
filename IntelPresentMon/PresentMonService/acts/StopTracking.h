#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include <format>
#include <unordered_set>

#define ACT_NAME StopTracking
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
			uint32_t targetPid;

			template<class A> void serialize(A& ar) {
				ar(targetPid);
			}
		};
		struct Response {};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			auto tpidIt = stx.trackedPids.find(in.targetPid);
			if (tpidIt == stx.trackedPids.end()) {
				pmlog_error("StopTracking called for untracked pid").pmwatch(in.targetPid);
				throw util::Except<ActionExecutionError>(PM_STATUS_INVALID_PID);
			}
			const bool wasBackpressured = tpidIt->second.isBackpressured;
			stx.trackedPids.erase(tpidIt);
			stx.frameReadProgress.erase(in.targetPid);
			// recompute effective serial now that this session no longer contributes
			if (wasBackpressured) {
				ctx.pPmon->GetBroadcaster().UpdateReadSerial(in.targetPid,
					ctx.ComputeEffectiveReadSerial_(in.targetPid));
			}
			ctx.pPmon->UpdateTracking(ctx.GetTrackedPidSet());
			pmlog_info(std::format("StopTracking action from [{}] un-targeting [{}]", stx.remotePid, in.targetPid));
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
