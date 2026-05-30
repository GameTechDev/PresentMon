#pragma once
#include <chrono>
#include <atomic>
#include <cstdint>
#include <limits>
#include <shared_mutex>
#include <unordered_map>

namespace pmon::util::log
{
	class LineTable
	{
	public:
		enum class ListMode
		{
			None,
			Black,
			White,
		};
		enum class TraceOverride
		{
			None,
			ForceOn,
			ForceOff,
		};
		struct Entry
		{
			uint32_t NextHit()
			{
				return ++hitCount_;
			}
			bool CheckTimeThrottle(std::chrono::nanoseconds duration, bool includeFirst)
			{
				using Clock = std::chrono::steady_clock;
				const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
					Clock::now().time_since_epoch()).count();

				auto lastPassNs = lastPassTimestampNs_.load(std::memory_order_relaxed);
				if (lastPassNs == kUninitializedTimestampNs_) {
					if (lastPassTimestampNs_.compare_exchange_strong(lastPassNs, nowNs, std::memory_order_relaxed)) {
						return includeFirst;
					}
				}

				while (true) {
					if ((nowNs - lastPassNs) < duration.count()) {
						return false;
					}
					if (lastPassTimestampNs_.compare_exchange_weak(lastPassNs, nowNs, std::memory_order_relaxed)) {
						return true;
					}
				}
			}
			uint32_t PeekHit() const
			{
				return hitCount_;
			}
			static constexpr int64_t kUninitializedTimestampNs_ = std::numeric_limits<int64_t>::min();
			std::atomic<uint32_t> hitCount_ = 0;
			std::atomic<int64_t> lastPassTimestampNs_ = kUninitializedTimestampNs_;
			bool isListed_ = false;
			TraceOverride traceOverride_ = TraceOverride::None;
		};
		static Entry* TryLookup(const std::string& file, int line) noexcept;
		static Entry& Lookup(const std::string& file, int line) noexcept;
		// blacklist, whitelist, or none
		static ListMode GetListMode() noexcept;
		static void SetListMode(ListMode mode) noexcept;
		// dictates whether line lookups necessary to check for per-line trace overrides
		static bool GetTraceOverride() noexcept;
		static void SetTraceOverride(bool) noexcept;
		static void RegisterListItem(const std::string& file, int line, TraceOverride traceOverride) noexcept;
		// returns true if there were any trace override lines, sets global trace and list settings based on contents
		static bool IngestList(const std::string& path, bool isBlacklist);

		// implementation functions
		static LineTable& Get_();
		LineTable::Entry* TryLookup_(const std::string& file, int line);
		Entry& Lookup_(const std::string& file, int line);
		void RegisterListItem_(const std::string& file, int line, TraceOverride traceOverride);
		bool GetTraceOverride_() const noexcept;
		void SetTraceOverride_(bool) noexcept;
		ListMode GetListMode_() const noexcept;
		void SetListMode_(ListMode mode) noexcept;
		bool IngestList_(const std::string& path, bool isBlacklist);
	private:
		// functions
		void RegisterListItem_(const std::string& key, TraceOverride traceOverride);
		static std::string MakeKey_(const std::string& file, int line);
		// data
		mutable std::shared_mutex mtx_;
		std::unordered_map<std::string, Entry> lines_;
		std::atomic<ListMode> listMode_ = ListMode::None;
		std::atomic<bool> traceOverride_ = false;
		static Entry dummyEntry_;
	};
}
