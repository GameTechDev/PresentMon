#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace pmon::ipc::act
{
	// Admission bounds for a multi-peer server. Every accepted connection is charged to
	// the identity of its client, so one identity can exhaust only its own allowance;
	// maxSessions remains a resource backstop across all identities. A session that has
	// not opened (set remotePid) within handshakeTimeoutMs is closed; once open it may
	// idle indefinitely. A single-peer server already admits only one session and
	// applies maxSessions alone.
	struct SessionLimits
	{
		uint32_t maxSessions = 64;
		// maximum allowed connections per user; further connections from that user are refused
		uint32_t maxSessionsPerIdentity = 8;
		uint32_t handshakeTimeoutMs = 3000;
	};
	static_assert(SessionLimits{}.maxSessionsPerIdentity < SessionLimits{}.maxSessions,
		"by default one identity must not be able to fill the server");

	// Maps the process id of an accepted client to the identity its sessions are charged
	// to. Throws when the identity cannot be determined, and the connection is refused.
	using ClientIdentityResolver = std::function<std::string(uint32_t clientPid)>;
}
