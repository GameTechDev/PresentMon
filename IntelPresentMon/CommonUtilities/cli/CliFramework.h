#pragma once

#pragma warning(push)
#pragma warning(disable : 26827)
#define CLI11_COMPILE 0
#include <CLI/CLI.hpp>
#pragma warning(pop)

#include <optional>
#include <cassert>
#include <string>
#include <span>
#include <sstream>

namespace pmon::util::cli
{
	// option elements inherit from this so we can enumerate options at runtime
	// and iterate over them (for child process forwarding etc.)
	// TODO: check how this interacts with std::vector etc. containers
	class OptionsElement_
	{
		friend struct RuleBase_;
		friend class OptionsContainer;
	public:
		virtual ~OptionsElement_() = default;
		virtual operator bool() const = 0;
	protected:
		std::function<std::string(std::string)> GetCaptureCallback_();
		void SetName_(std::string name);
	private:
		std::string name_;
		std::string raw_;
		bool forwarding_ = true;
	};

	class OptionsContainer
	{
		template<typename T> friend class Option;
		friend class Flag;
		friend struct RuleBase_;
	public:
		OptionsContainer(const char* description, const char* name);
		std::string GetName() const;
		std::vector<std::pair<std::string, std::string>> GetForwardedOptions() const;
	private:
		void AddGroup_(std::string name, std::string desc);
		void RegisterElement_(OptionsElement_* pElement);
	protected:
		// types
		class ConvertedNarrowOptions_
		{
		public:
			ConvertedNarrowOptions_(int argc, const wchar_t* const* wargv);
			~ConvertedNarrowOptions_();
			const char* const* GetRawPointerArray() const;

			ConvertedNarrowOptions_(const ConvertedNarrowOptions_&) = delete;
			ConvertedNarrowOptions_& operator=(const ConvertedNarrowOptions_&) = delete;
			ConvertedNarrowOptions_(ConvertedNarrowOptions_&&) = delete;
			ConvertedNarrowOptions_& operator=(ConvertedNarrowOptions_&&) = delete;

		private:
			std::vector<char*> stringPointerArray;
		};
		// functions
		void Finalize_(int argc, const char* const* argv);
		int Exit_(const CLI::ParseError& e, bool captureDiagnostics);
		// data
		std::ostringstream diagnostics_;
		std::vector<OptionsElement_*> elementPtrs_;
		bool finalized_ = false;
		std::string activeGroup_;
		CLI::App app_;
	};
	
	template<class T>
	class OptionsBase : public OptionsContainer
	{
	public:
		OptionsBase() : OptionsContainer{ T::description, T::name } {}
		static const T& Get()
		{
			auto& opts = Get_();
			assert(opts.finalized_);
			return opts;
		}
		static std::optional<int> Init(int argc, const wchar_t* const* wargv, bool captureDiagnostics = false)
		{
			ConvertedNarrowOptions_ narrowArgs{ argc, wargv };
			return Init(argc, narrowArgs.GetRawPointerArray(), captureDiagnostics);
		}
		static std::optional<int> Init(int argc, const char* const* argv, bool captureDiagnostics = false) noexcept
		{
			try {
				auto& opts = Get_();
				try {
					opts.Finalize_(argc, argv);
					return {};
				}
				catch (const CLI::ParseError& e) {
					return opts.Exit_(e, captureDiagnostics);
				}
			}
			catch (...) { return -1; }
		}
		static bool IsInitialized()
		{
			return Get_().finalized_;
		}
		static std::string GetDiagnostics()
		{
			return Get_().diagnostics_.str();
		}
	private:
		static T& Get_()
		{
			static T opts;
			return opts;
		}
	};

	template<typename T>
	class Option : public OptionsElement_
	{
		friend struct RuleBase_;
	public:
		template<class U>
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description, U&& customizer)
			:
			data_{ defaultValue }
		{
			// create the option
			pOption_ = pParent->app_.add_option(std::move(names), data_, std::move(description));
			// if customizer is a Validator object, add it to the option
			if constexpr (std::is_base_of_v<CLI::Validator, std::decay_t<U>> ) {
				if (customizer.get_modifying()) {
					pOption_->transform(std::forward<U>(customizer));
				}
				else {
					pOption_->check(std::forward<U>(customizer));
				}
			}
			else { // if customizer not a Validator, assume it's a function that works on the Option and call it
				customizer(pOption_);
			}
			OptionCommonPostCreate_(pParent);
		}
		Option(OptionsContainer* pParent, std::string names, const T& defaultValue, std::string description)
			:
			data_{ defaultValue }
		{
			// create the option
			pOption_ = pParent->app_.add_option(std::move(names), data_, std::move(description));
			OptionCommonPostCreate_(pParent);
		}
		Option(const Option&) = delete;
		Option& operator=(const Option&) = delete;
		Option(Option&&) = delete;
		Option& operator=(Option&&) = delete;
		~Option() = default;
		const T& operator*() const
		{
			return data_;
		}
		operator bool() const final
		{
			return (bool)*pOption_;
		}
		bool operator!() const
		{
			return !bool(*this);
		}
		std::optional<T> AsOptional() const
		{
			if (*this) {
				return **this;
			}
			return {};
		}
		std::string GetName() const
		{
			return pOption_->get_name();
		}
	private:
		// functions
		void OptionCommonPostCreate_(OptionsContainer* pParent)
		{
			// add to active group
			if (!pParent->activeGroup_.empty()) {
				pOption_->group(pParent->activeGroup_);
			}
			// capture main name for the option (used when forwarding)
			SetName_(pOption_->get_name());
			// capture the raw input string
			pOption_->transform(GetCaptureCallback_());
			// register this element with the container dynamically
			pParent->RegisterElement_(this);
		}
		// data
		T data_;
		CLI::Option* pOption_ = nullptr;
	};

	class Flag : public OptionsElement_
	{
		friend struct RuleBase_;
	public:
		Flag(OptionsContainer* pParent, std::string names, std::string description);
		bool operator*() const;
		operator bool() const final;
		bool operator!() const;
		std::string GetName() const
		{
			return pOption_->get_name();
		}
		bool IsPresent() const;
	private:
		bool data_ = false;
		CLI::Option* pOption_ = nullptr;
	};

	struct RuleBase_
	{
	protected:
		// TODO: could move pOption_ into OptionsElement_, would obviate this template
		template<class T>
		CLI::Option* GetOption_(T& el) const { return el.pOption_; }
		CLI::App& GetApp_(OptionsContainer& con) const { return con.app_; }
		void SetForwarding_(OptionsElement_& el, bool forwarding = false) { el.forwarding_ = forwarding; }
		void AddGroup_(OptionsContainer& con, std::string name, std::string desc = {}) { con.AddGroup_(std::move(name), std::move(desc)); }
	};

	class MutualExclusion : RuleBase_
	{
	public:
		template<class...T>
		MutualExclusion(T&...elements)
		{
			Exclude(elements...);
		}
	private:
		template<class T, class...Rest>
		void Exclude(T& pivot, const Rest&...rest) const
		{
			if constexpr (sizeof...(rest) > 0) {
				GetOption_(pivot)->excludes(GetOption_(rest)...);
				Exclude(rest...);
			}
		}
	};

	class Dependency : RuleBase_
	{
	public:
		template<class...T>
		Dependency(T&...elements)
		{
			Depend_(elements...);
		}
	private:
		template<class T, class...Rest>
		void Depend_(T& pivot, const Rest&...rest) const
		{
			GetOption_(pivot)->needs(GetOption_(rest)...);
		}
	};

	class NoForward : RuleBase_
	{
	public:
		template<class...T>
		NoForward(T&...elements)
		{
			(..., SetForwarding_(elements));
		}
	};

	class AllowExtras : RuleBase_
	{
	public:
		AllowExtras(OptionsContainer* pCon)
		{
			GetApp_(*pCon).allow_extras(true);
		}
	};

	class Group : RuleBase_
	{
	public:
		Group(OptionsContainer* pCon, std::string name, std::string desc = {})
		{
			AddGroup_(*pCon, std::move(name), std::move(desc));
		}
	};

	class SilentGroup : RuleBase_
	{
	public:
		SilentGroup(OptionsContainer* pCon)
		{
			AddGroup_(*pCon, {});
		}
	};
}