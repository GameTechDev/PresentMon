#pragma once
#include "../../../CommonUtilities/Exception.h"
#include <cstdint>

namespace pmon::ipc::act
{
	// Why an accepted session is ending. This is the transport cause and is not
	// rewritten just because the service has entered final teardown.
	enum class SessionEndReason
	{
		PeerDisconnected,
		ProtocolFailure,
		// the peer declared a packet length and then stopped sending its body
		PeerStalled,
		LocalShutdown,
	};

	// Whether disposal may recompute process-global service state.
	// Independent of SessionEndReason: a peer disconnect during final teardown
	// keeps PeerDisconnected and still suppresses ETW restart.
	enum class SessionCleanupMode : uint8_t
	{
		NormalOperation,
		FinalTeardown,
	};

	struct SessionDisposition
	{
		SessionEndReason reason;
		SessionCleanupMode cleanupMode = SessionCleanupMode::NormalOperation;
	};

	// Normal operation keeps the historical rule: every reason except LocalShutdown
	// recomputes global tracking and telemetry. Final teardown never does.
	inline bool ShouldRecomputeGlobalState(SessionDisposition disposition)
	{
		return disposition.cleanupMode == SessionCleanupMode::NormalOperation
			&& disposition.reason != SessionEndReason::LocalShutdown;
	}

	// A response that cannot be matched to a pending or still-retained expired
	// request. The body is drained before this is thrown so only this session ends.
	PM_DEFINE_EX(ProtocolViolation);
}
