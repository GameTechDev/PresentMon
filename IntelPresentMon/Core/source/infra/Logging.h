#pragma once
//#define PMLOG_BUILD_LEVEL Verbose
//#define VVV_V8ASYNC
#include <CommonUtilities/log/Log.h>


namespace p2c::v
{
#ifndef VVV_WINDOW // system that controls non-ui windows (overlays etc.)
		inline constexpr bool window = false;
#else
		inline constexpr bool window = true;
#endif
#ifndef VVV_PROCWATCH // system that tracks overlay target process ancestry and windows spawning therein
		inline constexpr bool procwatch = false;
#else
		inline constexpr bool procwatch = true;
#endif	
#ifndef VVV_OVERLAY // system that controls overlay operation
		inline constexpr bool overlay = false;
#else
		inline constexpr bool overlay = true;
#endif
#ifndef VVV_METRIC // system for parsing QualifiedMetric lists and building queries, fetchers, and data packs
		inline constexpr bool metric = false;
#else
		inline constexpr bool metric = true;
#endif
		inline constexpr bool hotkey2 = false;
}