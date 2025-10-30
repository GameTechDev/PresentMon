#pragma once
#include <CommonUtilities/log/Log.h>

namespace p2c::clog::p {
#ifdef PPP_OVERLAY
	inline static constexpr bool overlay = true;
#else
	inline static constexpr bool overlay = false;
#endif
}
