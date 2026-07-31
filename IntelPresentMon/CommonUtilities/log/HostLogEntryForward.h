#pragma once
#include "Entry.h"
#include "IChannel.h"
#include <utility>

namespace pmon::util::log
{
	// Must be called from the host module that owns pChannel (/MT heap). Middleware passes
	// this function pointer when linking logging across module boundaries.
	inline void ForwardLogEntryToHostChannel(IChannel* pChannel, const Entry& e) noexcept
	{
		if (!pChannel) {
			return;
		}
		try {
			Entry hostEntry;
			hostEntry.level_ = e.level_;
			hostEntry.subsystem_ = e.subsystem_;
			hostEntry.note_ = e.note_;
			hostEntry.sourceLine_ = e.sourceLine_;
			hostEntry.timestamp_ = e.timestamp_;
			hostEntry.errorCode_ = ErrorCode{ e.errorCode_ };
			hostEntry.pid_ = e.pid_;
			hostEntry.tid_ = e.tid_;
			hostEntry.rateControl_ = e.rateControl_;
			hostEntry.hitCount_ = e.hitCount_;
			hostEntry.diagnosticLayer_ = e.diagnosticLayer_;
			if (auto pStatic = std::get_if<Entry::StaticSourceStrings>(&e.sourceStrings_)) {
				hostEntry.sourceStrings_ = Entry::HeapedSourceStrings{
					pStatic->file_ ? pStatic->file_ : "",
					pStatic->functionName_ ? pStatic->functionName_ : "",
				};
			}
			else {
				const auto& heap = std::get<Entry::HeapedSourceStrings>(e.sourceStrings_);
				hostEntry.sourceStrings_ = Entry::HeapedSourceStrings{ heap.file_, heap.functionName_ };
			}
			hostEntry.pTrace_ = e.pTrace_;
			pChannel->Submit(std::move(hostEntry));
		}
		catch (...) {
		}
	}
}
