#pragma once
#include "SessionEndReason.h"

namespace pmon::ipc::act
{
	template<class E>
	concept HasCustomSessionDispose = requires(E ctx, typename E::SessionContextType stx, SessionDisposition disposition) {
		ctx.Dispose(stx, disposition);
	};
}