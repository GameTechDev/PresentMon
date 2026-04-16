#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include <format>

#define ACT_NAME ReportFrameReadProgress
#define ACT_EXEC_CTX ActionExecutionContext
#define ACT_NS ::pmon::svc::acts
#define ACT_TYPE AsyncEventActionBase_

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
			uint64_t nextReadSerial;

			template<class A> void serialize(A& ar) {
				ar(targetPid, nextReadSerial);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static void Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			// verify this session is tracking the target pid
			auto tpidIt = stx.trackedPids.find(in.targetPid);
			if (tpidIt == stx.trackedPids.end()) {
				pmlog_error("ReportFrameReadProgress for untracked pid").pmwatch(in.targetPid);
				throw util::Except<ActionExecutionError>(PM_STATUS_INVALID_PID);
			}
			// verify the target was registered with backpressure
			if (!tpidIt->second.isBackpressured) {
				pmlog_error("ReportFrameReadProgress for non-backpressured target").pmwatch(in.targetPid);
				throw util::Except<ActionExecutionError>(PM_STATUS_BAD_ARGUMENT);
			}
			// reject non-monotonic progress
			auto& sessionProgress = stx.frameReadProgress[in.targetPid];
			if (in.nextReadSerial < sessionProgress) {
				pmlog_error("ReportFrameReadProgress non-monotonic progress")
					.pmwatch(in.targetPid)
					.pmwatch(in.nextReadSerial)
					.pmwatch(sessionProgress);
				throw util::Except<ActionExecutionError>(PM_STATUS_BAD_ARGUMENT);
			}
			// reject progress greater than the service-owned write serial
			const auto writeSerial = ctx.pPmon->GetBroadcaster().GetCurrentWriteSerial(in.targetPid);
			if (!writeSerial || in.nextReadSerial > *writeSerial) {
				pmlog_error("ReportFrameReadProgress exceeds write serial")
					.pmwatch(in.targetPid)
					.pmwatch(in.nextReadSerial);
				throw util::Except<ActionExecutionError>(PM_STATUS_BAD_ARGUMENT);
			}
			// update service-owned progress for this session
			sessionProgress = in.nextReadSerial;
			// recompute effective minimum across all sessions and notify the producer
			ctx.pPmon->GetBroadcaster().UpdateReadSerial(in.targetPid,
				ctx.ComputeEffectiveReadSerial_(in.targetPid));
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
