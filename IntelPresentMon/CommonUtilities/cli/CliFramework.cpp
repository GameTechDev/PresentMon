#include "CliFramework.h"
#include "../str/String.h"
#include <span>
#include <vector>
#include <format>
#include <ranges>

namespace pmon::util::cli
{
	OptionsContainer::OptionsContainer(const char* description, const char* name) : app_{ description, name } {}
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
			app_.add_option_group({})->silent();
		}
		else {
			app_.add_option_group(std::move(name), std::move(desc));
		}
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

	OptionsContainer::ConvertedNarrowOptions_::ConvertedNarrowOptions_(int argc, const wchar_t* const* wargv)
	{
		for (auto pWideStr : std::span{ wargv, size_t(argc) }) {
			const auto narrow = str::ToNarrow(pWideStr);
			auto pNarrowStr = new char[narrow.size() + 1];
			strcpy_s(pNarrowStr, narrow.size() + 1, narrow.c_str());
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
		pOption_ = pParent->app_.add_flag(std::move(names), data_, std::move(description));
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