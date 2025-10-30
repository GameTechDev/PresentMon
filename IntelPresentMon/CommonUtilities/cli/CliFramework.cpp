#include "CliFramework.h"
#include "../str/String.h"
#include <span>
#include <vector>
#include <format>
#include <ranges>
#include <memory>

namespace pmon::util::cli
{
	void StripOptionGroupListFromHelp_(std::string& help)
	{
		const std::string kGroup = "[Option Group:";
		const std::string kSub = "SUBCOMMANDS:";

		auto begin = help.begin();
		auto end = help.end();

		// Find start of the first "[Option Group:"
		auto itStart = std::search(begin, end, kGroup.begin(), kGroup.end());
		if (itStart == end) return; // nothing to strip

		// Find the first "SUBCOMMANDS:" after that
		auto itEnd = std::search(itStart, end, kSub.begin(), kSub.end());

		// If not found, erase to end
		if (itEnd == end) {
			help.erase(static_cast<size_t>(itStart - begin));
			return;
		}

		// Trim trailing newlines before the header so we don't leave a blank gap
		auto itTrimEnd = itEnd;
		while (itTrimEnd != itStart) {
			auto prev = itTrimEnd - 1;
			if (*prev == '\n' || *prev == '\r') itTrimEnd = prev;
			else break;
		}

		help.erase(static_cast<size_t>(itStart - begin),
			static_cast<size_t>(itTrimEnd - itStart));
	}

	OptionsContainer::OptionsContainer(const char* description, const char* name) : app_{ description, name } {
		struct NoGroupListFormatter : CLI::Formatter {
			std::string make_footer(const CLI::App* app) const override {
				// Suppress entirely
				return "";
			}
		};
		app_.formatter(std::make_shared<NoGroupListFormatter>());
	}
	std::string OptionsContainer::GetName() const
	{
		return app_.get_name();
	}
	std::vector<std::pair<std::string, std::string>> OptionsContainer::GetForwardedOptions() const
	{
		std::vector<std::pair<std::string, std::string>> options;
		for (auto pEl : elementPtrs_) {
			if (pEl->forwarding_ && bool(*pEl)) {
				options.emplace_back(pEl->name_, pEl->raw_);
			}
		}
		return options;
	}
	void OptionsContainer::AddGroup_(std::string name, std::string desc)
	{
		activeGroup_ = name;
		if (name.empty()) {
			pCurrentSubcommand_->add_option_group({})->silent();
		}
		else {
			pCurrentSubcommand_->add_option_group(std::move(name), std::move(desc));
		}
	}
	CLI::App* OptionsContainer::AddSubcommand_(std::string name, std::string desc)
	{
		pCurrentSubcommand_ = app_.add_subcommand(std::move(name), std::move(desc));
		return pCurrentSubcommand_;
	}
	void OptionsContainer::RegisterElement_(OptionsElement_* pElement)
	{
		elementPtrs_.push_back(pElement);
	}
	void OptionsContainer::Finalize_(int argc, const char* const* argv)
	{
		app_.parse(argc, argv);
		finalized_ = true;
	}
	int OptionsContainer::Exit_(const CLI::ParseError& e, bool captureDiagnostics)
	{
		if (captureDiagnostics) {
			return app_.exit(e, diagnostics_, diagnostics_);
		}
		else {
			return app_.exit(e);
		}
	}
	std::string OptionsContainer::GetDiagnostics_() const
	{
		auto out = diagnostics_.str();
		StripOptionGroupListFromHelp_(out);
		return out;
	}

	OptionsContainer::ConvertedNarrowOptions_::ConvertedNarrowOptions_(int argc, const wchar_t* const* wargv)
	{
		for (auto pWideStr : std::span{ wargv, size_t(argc) }) {
			const auto narrow = str::ToNarrow(pWideStr);
			auto pNarrowStr = new char[narrow.size() + 1];
			strncpy_s(pNarrowStr, narrow.size() + 1, narrow.c_str(), _TRUNCATE);
			stringPointerArray.push_back(pNarrowStr);
		}
	}
	OptionsContainer::ConvertedNarrowOptions_::~ConvertedNarrowOptions_()
	{
		for (auto p : stringPointerArray) {
			delete[] p;
		}
	}
	const char* const* OptionsContainer::ConvertedNarrowOptions_::GetRawPointerArray() const
	{
		return stringPointerArray.data();
	}

	Flag::Flag(OptionsContainer* pParent, std::string names, std::string description)
	{
		// create the option
		pOption_ = pParent->pCurrentSubcommand_->add_flag(std::move(names), data_, std::move(description));
		// add to active group
		pOption_->group(pParent->activeGroup_);
		// capture main name for the option (used when forwarding)
		SetName_(pOption_->get_name());
		// register this element with the options container dynamically
		pParent->RegisterElement_(this);
	}
	bool Flag::operator*() const
	{
		return bool(*this);
	}
	Flag::operator bool() const
	{
		return data_;
	}
	bool Flag::operator!() const
	{
		return !bool(*this);
	}
	bool Flag::IsPresent() const
	{
		return (bool)*pOption_;
	}
	std::function<std::string(std::string)> OptionsElement_::GetCaptureCallback_()
	{
		return [this](std::string s) -> std::string {
			raw_ = s;
			return std::move(s);
		};
	}
	void OptionsElement_::SetName_(std::string name)
	{
		name_ = std::move(name);
	}
}