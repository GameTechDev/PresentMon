#pragma once
#include "SessionEndReason.h"
#include <concepts>
#include <cstdint>

namespace pmon::ipc::act
{
	// What the transport requires of a session context. The transport creates one per
	// session and reads remotePid for logging and the OpenSession check; the application
	// sets remotePid when the session opens. Everything else in it belongs to the application.
	template<class S>
	concept TransportSessionContext = std::default_initializable<S> && requires(S stx) {
		{ stx.remotePid } -> std::same_as<uint32_t&>;
	};

	template<class E>
	concept HasCustomSessionDispose = requires(E ctx, typename E::SessionContextType stx, SessionDisposition disposition) {
		ctx.Dispose(stx, disposition);
	};
}
