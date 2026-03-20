#include "EntryBuilder.h"
#include "IChannel.h"
#include "../str/String.h"
#include "../win/WinAPI.h"
#include "PanicLogger.h"
#include "StackTrace.h"
#include "GlobalPolicy.h"
#include "LineTable.h"
#include "../win/HrErrorCodeProvider.h"

#include "EntryCereal.h"
#include <cereal/archives/binary.hpp>
#include <sstream>
#include <cassert>


#ifndef NDEBUG
#define PM_LOG_DEFAULT_TRACE_SKIP 3
#else
#define PM_LOG_DEFAULT_TRACE_SKIP 1
#endif


namespace pmon::util::log
{
	EntryStream::EntryStream(EntryBuilder& builder)
		: builder_{ builder }
	{}
	EntryStream::~EntryStream()
	{
		builder_.note(str());
	}


	EntryBuilder::EntryBuilder(Level lvl, const char* sourceFile, const char* sourceFunctionName, int sourceLine) noexcept
		:
		Entry{
			.level_ = lvl,
			.sourceStrings_ = Entry::StaticSourceStrings{
				.file_ = sourceFile,
				.functionName_ = sourceFunctionName,
			},
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
			.pid_ = GetCurrentProcessId(),
			.tid_ = GetCurrentThreadId(),
		},
		traceSkipDepth_{ PM_LOG_DEFAULT_TRACE_SKIP }
	{}
	EntryBuilder::EntryBuilder(Level lvl, std::string sourceFile, std::string, int sourceLine) noexcept
		:
		Entry{
			.level_ = lvl,
			.sourceStrings_ = Entry::HeapedSourceStrings{
				.file_ = sourceFile,
			},
			.sourceLine_ = sourceLine,
			.timestamp_ = std::chrono::system_clock::now(),
			.pid_ = GetCurrentProcessId(),
			.tid_ = GetCurrentThreadId(),
		},
		traceSkipDepth_{ PM_LOG_DEFAULT_TRACE_SKIP }
	{
	}
	EntryBuilder& EntryBuilder::mark(const TimePoint& tp) noexcept
	{
		try {
			const auto now = std::chrono::high_resolution_clock::now();
			const auto duration = std::chrono::duration<double, std::milli>(now - tp.value).count();

			if (note_.empty()) {
				note_ += std::format("    Marked: {:.3f}ms", duration);
			}
			else {
				note_ += std::format("\n    Marked: {:.3f}ms", duration);
			}
		}
		catch (...) { pmlog_panic_("Failed to mark time in EntryBuilder"); }
		return *this;
	}

	EntryBuilder& EntryBuilder::note(std::string note) noexcept
	{
		note_ = std::move(note);
		return *this;
	}
	EntryBuilder& EntryBuilder::to(std::shared_ptr<IEntrySink> pSink) noexcept
	{
		pDest_ = std::move(pSink);
		return *this;
	}
	EntryBuilder& EntryBuilder::trace_skip(int depth) noexcept
	{
		traceSkipDepth_ = depth;
		return *this;
	}
	EntryBuilder& EntryBuilder::no_trace() noexcept
	{
		captureTrace_ = false;
		return *this;
	}
	EntryBuilder& EntryBuilder::trace() noexcept
	{
		captureTrace_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::hr() noexcept
	{
		return hr(GetLastError());
	}
	EntryBuilder& EntryBuilder::hr(uint32_t hres) noexcept
	{
		return code(win::hr_wrap{ hres });
	}
	EntryBuilder& EntryBuilder::every(int n, bool includeFirst) noexcept
	{
		assert(n > 0);
		if (n > 1) {
			rateControl_ = {
				.type = includeFirst ? RateControl::Type::EveryAndFirst : RateControl::Type::Every,
				.parameter = n,
			};
		}
		else {
			rateControl_ = {};
		}
		return *this;
	}
	EntryBuilder& EntryBuilder::every(std::chrono::nanoseconds duration, bool includeFirst) noexcept
	{
		assert(duration.count() > 0);
		if (duration > std::chrono::nanoseconds::zero()) {
			rateControl_ = {
				.type = includeFirst ? RateControl::Type::EveryDurationAndFirst : RateControl::Type::EveryDuration,
				.duration = duration,
			};
		}
		else {
			rateControl_ = {};
		}
		return *this;
	}
	EntryBuilder& EntryBuilder::first(int n) noexcept
	{
		assert(n > 0);
		rateControl_ = {
			.type = RateControl::Type::First,
			.parameter = n,
		};
		return *this;
	}
	EntryBuilder& EntryBuilder::after(int n) noexcept
	{
		assert(n > 0);
		rateControl_ = {
			.type = RateControl::Type::After,
			.parameter = n,
		};
		return *this;
	}
	EntryBuilder& EntryBuilder::hitcount() noexcept
	{
		rateControl_ = { .type = RateControl::Type::Hitcount };
		return *this;
	}
	EntryBuilder& EntryBuilder::diag() noexcept
	{
		diagnosticLayer_ = true;
		return *this;
	}
	EntryBuilder& EntryBuilder::subsys(Subsystem sys) noexcept
	{
		subsystem_ = sys;
		return *this;
	}
	EntryStream EntryBuilder::stream() noexcept
	{
		return EntryStream{ *this };
	}
	void EntryBuilder::commit_() noexcept
	{
		try {
			if (pDest_) {
				auto tracing = captureTrace_.value_or((int)level_ <= (int)GlobalPolicy::Get().GetTraceLevel());
				// do line override check
				if (LineTable::GetTraceOverride()) {
					if (auto pEntry = LineTable::TryLookup(GetSourceFileName(), sourceLine_)) {
						switch (pEntry->traceOverride_) {
						case LineTable::TraceOverride::ForceOn: tracing = true; break;
						case LineTable::TraceOverride::ForceOff: tracing = false; break;
						}
					}
				}
				if (tracing) {
					try {
						pTrace_ = StackTrace::Here(traceSkipDepth_);
						if (GlobalPolicy::Get().GetResolveTraceInClientThread()) {
							pTrace_->Resolve();
						}
					}
					catch (...) {
						pmlog_panic_("Failed to get current stacktrace");
					}
				}
				pDest_->Submit(std::move(*this));
			}
			else {
				pmlog_panic_("Log entry completed with no destination channel set");
			}
		}
		catch (...) {
			pmlog_panic_("Error when completing log entry");
		}
		committed_ = true;
	}
	EntryBuilder::~EntryBuilder()
	{
		if (!committed_) {
			commit_();
		}
	}
}
