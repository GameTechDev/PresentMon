#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "../../CommonUtilities/win/Utilities.h"
#include <format>
#include <unordered_set>

#define ACT_NAME StartTracking
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
			bool isPlayback = false;
			bool isBackpressured = false;

			template<class A> void serialize(A& ar) {
				ar(targetPid, isPlayback, isBackpressured);
			}
		};
		struct Response
		{
			template<class A> void serialize(A& ar) {
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			// playback mode compatibility check
			const bool serviceIsPlayback = ctx.pPmon->IsPlayback();
			if (serviceIsPlayback != in.isPlayback) {
				pmlog_error("StartTracking playback mode mismatch")
					.pmwatch(serviceIsPlayback).pmwatch(in.isPlayback);
				throw util::Except<ActionExecutionError>(PM_STATUS_MODE_MISMATCH);
			}
			// backpressure requires playback mode
			if (!in.isPlayback && in.isBackpressured) {
				pmlog_error("StartTracking backpressured requires isPlayback=true");
				throw util::Except<ActionExecutionError>(PM_STATUS_BAD_ARGUMENT);
			}
			// check if this session already tracking requested pid
			if (stx.trackedPids.find(in.targetPid) != stx.trackedPids.end()) {
				pmlog_error("StartTracking called for already tracked pid").pmwatch(in.targetPid);
				throw util::Except<ActionExecutionError>(PM_STATUS_ALREADY_TRACKING_PROCESS);
			}
			// lock in handle to process for pid parking purposes
			ActionSessionContext::TrackedTarget target{};
			if (!in.isPlayback) {
				target.processHandle = util::win::OpenProcess(in.targetPid);
				if (!target.processHandle) {
					pmlog_error("StartTracking called for invalid pid").pmwatch(in.targetPid);
					throw util::Except<ActionExecutionError>(PM_STATUS_INVALID_PID);
				}
			}
			// build full tracking state for session sync
			auto trackedPids = ctx.GetTrackedPidSet();
			trackedPids.emplace(in.targetPid);
			// get the (possibly shared) segment (new or find operation in broadcaster)
			auto pSegment = ctx.pPmon->GetBroadcaster().RegisterTarget(
				in.targetPid, in.isPlayback, in.isBackpressured);
			// the segment may have been created by an earlier session with a different mode;
			// reject the join rather than silently recording backpressure state that will
			// never take effect (Push() only blocks when backpressured_ was set at construction)
			if (pSegment->GetStore().frameData.IsBackpressured() != in.isBackpressured) {
				pmlog_error("StartTracking backpressure mode conflicts with existing segment")
					.pmwatch(in.targetPid)
					.pmwatch(in.isBackpressured);
				throw util::Except<ActionExecutionError>(PM_STATUS_BAD_ARGUMENT);
			}
			if (auto sta = ctx.pPmon->UpdateTracking(trackedPids); sta != PM_STATUS_SUCCESS) {
				pmlog_error("Start tracking call failed").code(sta);
				throw util::Except<ActionExecutionError>(sta);
			}
			// initialize service-owned backpressure state before the target is recorded
			if (in.isBackpressured) {
				const auto initialSerial = pSegment->GetStore().frameData.GetSerialRange().first;
				stx.frameReadProgress[in.targetPid] = initialSerial;
			}
			target.isBackpressured = in.isBackpressured;
			target.pSegment = std::move(pSegment);
			stx.trackedPids.emplace(in.targetPid, std::move(target));
			// update the ring with the new effective minimum (may lower it if this session
			// joins with an initial serial below the current ring value)
			if (in.isBackpressured) {
				ctx.pPmon->GetBroadcaster().UpdateReadSerial(in.targetPid,
					ctx.ComputeEffectiveReadSerial_(in.targetPid));
			}
			pmlog_info(std::format("StartTracking action from [{}] targeting [{}]",
				stx.remotePid, in.targetPid));
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
