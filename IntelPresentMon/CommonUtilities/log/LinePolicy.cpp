#include "LinePolicy.h"
#include "Entry.h"
#include "LineTable.h"
#include <format>

namespace pmon::util::log
{
	bool LinePolicy::TransformFilter(Entry& e)
	{
		LineTable::Entry* pEntry = nullptr;
		const auto listMode = LineTable::GetListMode();
		// processing for black/whitelists
		// first check if listing is active
		if (listMode != LineTable::ListMode::None) {
			// check if this line is listed
			if (pEntry = LineTable::TryLookup(e.GetSourceFileName(), e.sourceLine_); pEntry && pEntry->isListed_) {
				// in blacklist mode, lines listed are dropped
				if (listMode == LineTable::ListMode::Black) {
					return false;
				}
			}
			else {
				// in whitelist mode, lines not listed are dropped
				if (listMode == LineTable::ListMode::White) {
					return false;
				}
			}
		}
		if (e.rateControl_.type != Entry::RateControl::Type::None) {
			// lookup the line entry, use previous lookup results if available from list processing
			auto& te = pEntry ? *pEntry : LineTable::Lookup(e.GetSourceFileName(), e.sourceLine_);
			e.hitCount_ = te.NextHit();
			switch (e.rateControl_.type) {
			case Entry::RateControl::Type::After:
				if (e.hitCount_ <= e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::Every:
				if (e.hitCount_ % e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::EveryAndFirst:
				if ((e.hitCount_ - 1) % e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::EveryDuration:
				if (!te.CheckTimeThrottle(e.rateControl_.duration, false)) return false;
				break;
			case Entry::RateControl::Type::EveryDurationAndFirst:
				if (!te.CheckTimeThrottle(e.rateControl_.duration, true)) return false;
				break;
			case Entry::RateControl::Type::First:
				if (e.hitCount_ > e.rateControl_.parameter) return false;
				break;
			case Entry::RateControl::Type::Hitcount:
			case Entry::RateControl::Type::None:
				break;
			}
		}
		return true;
	}
}
