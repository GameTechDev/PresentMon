#pragma once

#pragma warning(push)
#pragma warning(disable : 26827)
#include <CLI/CLI.hpp>
#pragma warning(pop)

#include <optional>
#include <stdexcept>
#include <type_traits>

namespace p2c::infra::opt::impl
{
	struct OptionStructBase_
	{
	protected:
		friend class AppBase_;
		virtual ~OptionStructBase_() = default;
		virtual std::string GetDescription() const = 0;
		virtual std::string GetName() const = 0;
	};

	class IEnumeratedOption_
	{
	public:
		struct Results_
		{
			std::string name;
			std::string value;
			bool present = false;
		};
		virtual Results_ GetResults() const = 0;
		virtual ~IEnumeratedOption_() = default;
	};

	// used to forward options to child processes
	struct ForwardingOption
	{
		std::string name;
		std::string value;
	};

	class AppBase_
	{
		template<typename T>
		friend class Option;
		friend class Flag;
		friend class Group;
	protected:
		AppBase_() = default;
		static void InitInternal_(int argc, char** argv);
		static CLI::App& GetApp_();
		static void Finalize_(const OptionStructBase_&);
		static void RegisterOption_(const IEnumeratedOption_* pOption);
		static std::vector<ForwardingOption> GetForwardingOptions();
	private:
		// types
		struct CommandLine
		{
			int argc;
			char** argv;
		};
		// data
		static std::optional<CLI::App> app_;
		static std::optional<CommandLine> line_;
		static const char* group_;
		static std::vector<const IEnumeratedOption_*> enumeratedOptionPtrs_;
	};

	template<class T> concept OptionStructable = std::derived_from<T, OptionStructBase_>;

	template<OptionStructable OptionStruct>
	class App : public AppBase_
	{
	public:
		static void Init(int argc, char** argv)
		{
			InitInternal_(argc, argv);
			GetOptions();
		}
		static OptionStruct& GetOptions()
		{
			static App singleton;
			return singleton.options_;
		}
		static bool CheckOptionPresence(const std::string& key)
		{
			const auto pOption = GetApp_()[key];
			return pOption && pOption->count() > 0;
		}
		static std::vector<ForwardingOption> GetForwardingOptions()
		{
			return AppBase_::GetForwardingOptions();
		}
	private:
		App() { Finalize_(options_); }
		OptionStruct options_;
	};

	class Group
	{
	public:
		Group(const char* name);
	private:
		OptionStructBase_* optionStruct_ = nullptr;
	};

	template<typename T>
	class Option : public IEnumeratedOption_
	{
	public:
		template<class U>
		Option(std::string names, std::string description, U&& customizer, bool forward = true)
			:
			forward_{ forward }
		{
			pOption_ = AppBase_::GetApp_().add_option(std::move(names), value_, std::move(description));
			if (AppBase_::group_) {
				pOption_->group(AppBase_::group_);
			}
			// if the customizer returns a T, it is the default value
			using R = std::invoke_result_t<U, CLI::Option*>;
			if constexpr (std::is_same_v<R, T>) {
				value_ = customizer(pOption_);
			}
			else {
				customizer(pOption_);
			}
			AppBase_::RegisterOption_(this);
		}
		Option(std::string names, std::string description, bool forward = true)
			:
			forward_{ forward }
		{
			pOption_ = AppBase_::GetApp_().add_option(std::move(names), value_, std::move(description));
			if (AppBase_::group_) {
				pOption_->group(AppBase_::group_);
			}
			AppBase_::RegisterOption_(this);
		}
		IEnumeratedOption_::Results_ GetResults() const override
		{
			Results_ r;
			r.name = pOption_->get_name();
			if (r.present = bool(*this) && forward_) {
				if constexpr (std::same_as<T, std::string>) {
					r.value = '"' + value_ + '"';
				}
				else {
					r.value = std::to_string(value_);
				}
			}
			return r;
		}
		operator bool() const
		{
			return (bool)*pOption_;
		}
		bool operator!() const
		{
			return !bool(*this);
		}
		const T& operator*() const
		{
			return value_;
		}
		CLI::Option* opt() const { return pOption_; }
	private:
		T value_{};
		CLI::Option* pOption_ = nullptr;
		bool forward_;
	};

	class Flag : public IEnumeratedOption_
	{
	public:
		template<class U>
		Flag(std::string names, std::string description, U&& customizer, bool forward = true)
			:
			forward_{ forward }
		{
			pOption_ = AppBase_::GetApp_().add_flag(std::move(names), value_, std::move(description));
			if (AppBase_::group_) {
				pOption_->group(AppBase_::group_);
			}
			// if the customizer returns a bool, it is the default value
			using R = std::invoke_result_t<U, CLI::Option*>;
			if constexpr (std::is_same_v<R, bool>) {
				value_ = customizer(pOption_);
			}
			else {
				customizer(pOption_);
			}
			AppBase_::RegisterOption_(this);
		}
		Flag(std::string names, std::string description, bool forward = true);
		IEnumeratedOption_::Results_ GetResults() const override
		{
			Results_ r;
			r.name = pOption_->get_name();
			r.present = bool(*this) && forward_;
			return r;
		}
		operator bool() const;
		CLI::Option* opt() const;
	private:
		// data
		bool value_ = false;
		CLI::Option* pOption_ = nullptr;
		bool forward_;
	};
}