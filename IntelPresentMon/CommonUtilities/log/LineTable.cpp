#include "LineTable.h"
#include <fstream>
#include <regex>
#include "../str/String.h"
#include "Log.h"
#include <ranges>
#include <vector>

namespace pmon::util::log
{
	LineTable::Entry LineTable::dummyEntry_ = {};

	LineTable::Entry* LineTable::TryLookup(const std::string& file, int line) noexcept
	{
		try {
			return Get_().TryLookup_(file, line);
		}
		catch (...) { return nullptr; }
	}
	LineTable::Entry& LineTable::Lookup(const std::string& file, int line) noexcept
	{
		try {
			return Get_().Lookup_(file, line);
		}
		catch (...) { return dummyEntry_; }
	}
	LineTable::ListMode LineTable::GetListMode() noexcept
	{
		try {
			return Get_().listMode_;
		}
		catch (...) { return ListMode::None; }
	}
	void LineTable::SetListMode(ListMode mode) noexcept
	{
		try {
			Get_().listMode_ = mode;
		}
		catch (...) {}
	}
	bool LineTable::GetTraceOverride() noexcept
	{
		try {
			return Get_().traceOverride_;
		}
		catch (...) { return false; }
	}
	void LineTable::SetTraceOverride(bool ov) noexcept
	{
		try {
			Get_().traceOverride_ = ov;
		}
		catch (...) {}
	}
	void LineTable::RegisterListItem(const std::string& file, int line, TraceOverride traceOverride) noexcept
	{
		try {
			Get_().RegisterListItem_(file, line, traceOverride);
		}
		catch (...) {}
	}

	bool LineTable::IngestList_(const std::string& path, bool isBlacklist)
	{
		ReplaceList_(path, isBlacklist ? ListMode::Black : ListMode::White);
		return traceOverride_;
	}
		
	bool LineTable::IngestList(const std::string& path, bool isBlacklist)
	{
		return Get_().IngestList_(path, isBlacklist);
	}

	void LineTable::ReplaceList(const std::filesystem::path& path, ListMode mode)
	{
		Get_().ReplaceList_(path, mode);
	}

	void LineTable::ClearList() noexcept
	{
		try {
			Get_().ClearList_();
		}
		catch (...) {}
	}

	void LineTable::ReplaceList_(const std::filesystem::path& path, ListMode mode)
	{
		struct ParsedEntry_
		{
			std::string key;
			TraceOverride traceOverride;
		};
		std::vector<ParsedEntry_> parsed;
		bool hasTraceOverride = false;
		bool hasListedEntry = false;
		std::ifstream listFile{ path };
		if (!listFile) {
			throw std::runtime_error{ "Failed to open list file" };
		}
		std::string line;
		while (std::getline(listFile, line)) {
			line = str::TrimWhitespace(line);
			if (line.empty()) {
				continue;
			}
			auto traceOverride = TraceOverride::None;
			if (line.back() == '%') {
				traceOverride = TraceOverride::ForceOff;
				line.pop_back();
			}
			else if (line.back() == '$') {
				traceOverride = TraceOverride::ForceOn;
				line.pop_back();
			}
			std::regex patternLogged(R"((.+)\((\d+)\)$)");
			std::smatch matches;
			if (std::regex_search(line, matches, patternLogged) && matches.size() == 3) {
				parsed.push_back({
					.key = MakeKey_(matches[1], std::stoi(matches[2])),
					.traceOverride = traceOverride,
				});
				hasTraceOverride |= traceOverride != TraceOverride::None;
				hasListedEntry |= traceOverride == TraceOverride::None;
			}
		}
		if (listFile.bad()) {
			throw std::runtime_error{ "Failed while reading list file" };
		}

		// TODO: Use immutable snapshots or lifetime-safe lookups before supporting runtime
		// list updates; current callers configure once at startup, so this race is latent.
		std::lock_guard lk{ mtx_ };
		lines_.reserve(lines_.size() + parsed.size());
		for (const auto& parsedEntry : parsed) {
			lines_.try_emplace(parsedEntry.key);
		}
		for (auto& entry : lines_ | std::views::values) {
			entry.isListed_ = false;
			entry.traceOverride_ = TraceOverride::None;
		}
		for (const auto& parsedEntry : parsed) {
			auto& entry = lines_.at(parsedEntry.key);
			entry.traceOverride_ = parsedEntry.traceOverride;
			entry.isListed_ = parsedEntry.traceOverride == TraceOverride::None;
		}
		traceOverride_ = hasTraceOverride;
		// only enable list filtering when the list has listed (non-trace) lines; an allow
		// list without any would otherwise drop every entry
		listMode_ = hasListedEntry ? mode : ListMode::None;
	}

	void LineTable::ClearList_()
	{
		std::lock_guard lk{ mtx_ };
		for (auto& entry : lines_ | std::views::values) {
			entry.isListed_ = false;
			entry.traceOverride_ = TraceOverride::None;
		}
		traceOverride_ = false;
		listMode_ = ListMode::None;
	}

	LineTable& LineTable::Get_()
	{
		// @SINGLETON
		static LineTable table;
		return table;
	}
	LineTable::Entry* LineTable::TryLookup_(const std::string& file, int line)
	{
		std::shared_lock lk{ mtx_ };
		if (auto i = lines_.find(MakeKey_(file, line)); i != lines_.end()) {
			return &i->second;
		}
		return nullptr;
	}
	LineTable::Entry& LineTable::Lookup_(const std::string& file, int line)
	{
		std::shared_lock lk{ mtx_ };
		return lines_[MakeKey_(file, line)];
	}
	void LineTable::RegisterListItem_(const std::string& file, int line, TraceOverride traceOverride)
	{
		RegisterListItem_(MakeKey_(file, line), traceOverride);
	}
	bool LineTable::GetTraceOverride_() const noexcept
	{
		return traceOverride_;
	}
	void LineTable::SetTraceOverride_(bool on) noexcept
	{
		traceOverride_ = on;
	}
	LineTable::ListMode LineTable::GetListMode_() const noexcept
	{
		return listMode_;
	}
	void LineTable::SetListMode_(ListMode mode) noexcept
	{
		listMode_ = mode;
	}
	void LineTable::RegisterListItem_(const std::string& key, TraceOverride traceOverride)
	{
		std::lock_guard lk{ mtx_ };
		auto& e = lines_[key];
		if (traceOverride != TraceOverride::None) {
			e.traceOverride_ = traceOverride;
			traceOverride_ = true;
		}
		else {
			e.isListed_ = true;
		}
	}
	std::string LineTable::MakeKey_(const std::string& file, int line)
	{
		return file + "?" + std::to_string(line);
	}
}