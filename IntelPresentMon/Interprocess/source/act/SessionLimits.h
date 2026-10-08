#pragma once
#include <cstdint>

namespace pmon::ipc::act
{
	// Admission bounds for a server. maxSessions caps concurrent sessions. On a multi-peer
	// server, a session that has not opened (set remotePid) within handshakeTimeoutMs is
	// closed; once open it may idle indefinitely. A single-peer server already admits only
	// one session and applies no handshake deadline.
	struct SessionLimits
	{
		uint32_t maxSessions = 64;
		uint32_t handshakeTimeoutMs = 3000;
	};
}
