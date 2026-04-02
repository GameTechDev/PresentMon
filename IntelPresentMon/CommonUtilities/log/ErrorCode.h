#pragma once
#include <variant>
#include <cstdint>
#include <concepts>
#include <optional>
#include <memory>
#include <string>
#include <typeindex>
#include "IErrorCodeResolver.h"
#include "../Memory.h"

namespace pmon::util::log
{
	template<typename T>
	concept IsWrappedErrorCode = requires (T a) {
		{ a.pmlog_code };
	};

	template<typename T>
	concept IsErrorCodeCompatible = IsWrappedErrorCode<std::decay_t<T>> ||
		std::is_enum_v<std::decay_t<T>> || std::integral<std::decay_t<T>>;

	template<class Archive>
	void serialize(Archive& ar, class ErrorCode& c);

	class ErrorCode
	{
		template<class Archive>
		friend void serialize(Archive& ar, ErrorCode& c);
	public:
		ErrorCode() = default;
		template<IsErrorCodeCompatible T>
		ErrorCode(const T& code)
		{
			using D = std::decay_t<T>;
			if constexpr (std::is_enum_v<D>) {
				pTypeInfo_ = &typeid(D);
				if constexpr (std::signed_integral<std::underlying_type_t<D>>) {
					integral_ = (int64_t)code;
				}
				else {
					integral_ = (uint64_t)code;
				}
			}
			else if constexpr (IsWrappedErrorCode<D>) {
				pTypeInfo_ = &typeid(D);
				if constexpr (std::signed_integral<decltype(code.pmlog_code)>) {
					integral_ = (int64_t)code.pmlog_code;
				}
				else {
					integral_ = (uint64_t)code.pmlog_code;
				}
			}
			else {
				if constexpr (std::signed_integral<D>) {
					integral_ = (int64_t)code;
				}
				else {
					integral_ = (uint64_t)code;
				}
			}
		}
		ErrorCode(const ErrorCode&);
		ErrorCode(ErrorCode&&);
		ErrorCode& operator=(ErrorCode&&);

		ErrorCode & operator=(const ErrorCode&) = delete;
		~ErrorCode() = default;

		bool HasUnsigned() const noexcept;
		bool HasSigned() const noexcept;
		bool HasIntegral() const noexcept;
		bool Fits32() const noexcept;
		std::optional<uint64_t> AsUnsigned() const noexcept;
		std::optional<int64_t> AsSigned() const noexcept;
		bool Empty() const noexcept;
		bool Resolve(const IErrorCodeResolver& resolver);
		bool HasTypeInfo() const noexcept;
		bool IsResolved() const noexcept;
		bool IsResolvedNontrivial() const noexcept;
		std::string AsHex() const;
		const IErrorCodeResolver::Strings* GetStrings() const;
		operator bool() const noexcept;
	private:
		const std::type_info* pTypeInfo_ = nullptr;
		CloningUptr<IErrorCodeResolver::Strings> pStrings_;
		std::variant<std::monostate, int64_t, uint64_t> integral_;
	};
}
