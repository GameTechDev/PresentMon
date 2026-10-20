#pragma once
#include "../../../CommonUtilities/Exception.h"
#include <cstdint>

namespace pmon::ipc::act
{
	// Allocates command tokens for one direction of one session.
	// A token is a routing id, not an authentication or authority value.
	// Construction from an explicit starting value is the seam tests use to
	// exercise wraparound; production callers keep the default of zero.
	class CommandTokenAllocator
	{
	public:
		CommandTokenAllocator() = default;
		explicit CommandTokenAllocator(uint32_t next)
			:
			next_{ next }
		{}
		// Skips values for which unavailable(token) is true so a wrap cannot
		// collide with a pending request or a retained expiration record.
		// Throws if no free value is found within the scan bound.
		template<class Unavailable>
		uint32_t Allocate(Unavailable&& unavailable)
		{
			constexpr uint32_t kMaxScan = 65536;
			for (uint32_t attempt = 0; attempt < kMaxScan; ++attempt) {
				const auto token = next_++;
				if (!unavailable(token)) {
					return token;
				}
			}
			throw pmon::util::Except<pmon::util::Exception>("Command token space exhausted");
		}
	private:
		uint32_t next_ = 0;
	};
}
