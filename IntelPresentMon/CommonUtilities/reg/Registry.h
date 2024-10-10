#pragma once

#include <winreg/winreg.hpp>

#include <optional>
#include <string>
#include <span>
#include "../Exception.h"
#include "../log/Log.h"
#include "../str/String.h"


namespace pmon::util::reg
{
	PM_DEFINE_EX(RegistryNotOpen);

	class Registry
	{
	public:
		// types
		template<typename T>
		class Value
		{
		public:
			Value(Registry* pParent, const std::string& valueName)
				:
				parent_{ *pParent },
				valueName_{ str::ToWide(valueName) }
			{}
			std::optional<T> AsOptional() const noexcept
			{
				try {
					return Get();
				}
				catch (const RegistryNotOpen&) {
					pmlog_warn("Optional access of registry value when key not open");
				}
				catch (const winreg::RegException& e) {
					if (e.code().value() == 2) {
						pmlog_dbg("Optional access of absent value");
					}
					else {
						pmlog_error(ReportException("Error during optional access of registry value"));
					}
				}
				catch (...) {
					pmlog_error(ReportException("Unknown error during optional access of registry value"));
				}
				return {};
			}
			T Get() const
			{
				auto& key = GetKey_();
				if constexpr (std::same_as<bool, T>) {
					return (bool)key.GetDwordValue(valueName_);
				}
				else if constexpr (std::integral<T>) {
					if constexpr (sizeof(T) <= 4) {
						return (T)key.GetDwordValue(valueName_);
					}
					else {
						return (T)key.GetQwordValue(valueName_);
					}
				}
				else if constexpr (std::is_enum_v<T>) {
					using U = std::underlying_type_t<T>;
					if constexpr (sizeof(U) <= 4) {
						return (T)key.GetDwordValue(valueName_);
					}
					else {
						return (T)key.GetQwordValue(valueName_);
					}
				}
				else if constexpr (std::same_as<std::string, T>) {
					return str::ToNarrow(key.GetStringValue(valueName_));
				}
			}
			void Set(const T& val)
			{
				auto& key = GetKey_();
				if constexpr (std::same_as<bool, T>) {
					return key.SetDwordValue(valueName_, (DWORD)val);
				}
				else if constexpr (std::integral<T>) {
					if constexpr (sizeof(T) <= 4) {
						return key.SetDwordValue(valueName_, (DWORD)val);
					}
					else {
						return key.SetQwordValue(valueName_, (ULONGLONG)val);
					}
				}
				else if constexpr (std::is_enum_v<T>) {
					using U = std::underlying_type_t<T>;
					if constexpr (sizeof(U) <= 4) {
						return key.SetDwordValue(valueName_, (DWORD)val);
					}
					else {
						return key.SetQwordValue(valueName_, (ULONGLONG)val);
					}
				}
				else if constexpr (std::same_as<std::string, T>) {
					key.SetStringValue(valueName_, str::ToWide(val));
				}
			}
			bool Exists() const
			{
				if (!parent_.key_.IsValid()) {
					pmlog_warn(ReportException("Attempting to check validity of value whose key is not open"));
					return false;
				}
				for (auto& v : parent_.key_.EnumValues()) {
					if (v.first == valueName_) {
						return true;
					}
				}
				return false;
			}
			void Delete() noexcept
			{
				try {
					parent_.key_.DeleteValue(valueName_);
				}
				catch (...) {
					pmlog_warn(ReportException("Failed to delete registry value"));
				}
			}
			operator T() const
			{
				return Get();
			}
			Value& operator=(const T& val)
			{
				Set(val);
				return *this;
			}
		private:
			// functions
			winreg::RegKey& GetKey_() const
			{
				auto& key = parent_.key_;
				if (!key.IsValid()) {
					throw Except<RegistryNotOpen>();
				}
				return key;
			}
			// data
			std::wstring valueName_;
			Registry& parent_;
		};
		// functions
		bool IsValid() const
		{
			return key_.IsValid();
		}
	protected:
		Registry(HKEY hive, std::wstring keyPath) noexcept
			:
			hive_{ hive },
			keyPath_{ keyPath }
		{
			// opening existing key
			try {
				key_.Open(hive_, keyPath_);
				return;
			}
			catch (const winreg::RegException& e) {
				if (e.code().value() == 2) {
					pmlog_warn(std::format("Registry key [{}] does not exist, creating...", str::ToNarrow(keyPath)));
				}
				else {
					pmlog_error(ReportException(std::format("Failed to open registry key [{}]", str::ToNarrow(keyPath))));
					return;
				}
			}
			catch (...) {
				pmlog_error(ReportException(std::format("Failed to open registry key [{}]", str::ToNarrow(keyPath))));
				return;
			}
			// creating new key
			try {
				key_.Create(hive_, keyPath_);
			}
			catch (...) {
				pmlog_error(ReportException(std::format("Failed to create registry key [{}]", str::ToNarrow(keyPath))));
			}
		}
	private:
		HKEY hive_;
		std::wstring keyPath_;
		winreg::RegKey key_;
	};

	template<class T>
	class RegistryBase : public Registry
	{
	public:
		static T& Get() noexcept
		{
			static T reg;
			return reg;
		}
		static void SetPrivileged(bool privileged) noexcept
		{
			privileged_ = privileged;
		}
	protected:
		RegistryBase() : Registry{ privileged_ ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, T::keyPath_ } {}
	private:
		inline static bool privileged_ = true;
	};
}