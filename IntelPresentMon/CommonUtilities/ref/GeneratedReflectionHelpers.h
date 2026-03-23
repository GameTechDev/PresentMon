#pragma once
#include <string>
#include <concepts>
#include "GeneratedReflection.h"

namespace pmon::util::ref::gen
{
	namespace {
		using namespace std::literals;

		template<typename T, size_t N>
		std::string DumpArray_(const T(&arr)[N])
		{
			std::ostringstream oss;
			oss << "[";
			if constexpr (std::is_array_v<T>) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << DumpArray_(arr[i]) << ",";
				}
			}
			else if constexpr (std::same_as<T, char>) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << arr[i] << ",";
				}
			}
			else if constexpr (std::same_as<T, unsigned char>) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << (int)arr[i] << ",";
				}
			}
			else if constexpr (
				std::same_as<T, signed char> ||
				std::same_as<T, wchar_t> ||
				std::same_as<T, char8_t> ||
				std::same_as<T, char16_t> ||
				std::same_as<T, char32_t>) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << (uint32_t)arr[i] << ",";
				}
			}
			else if constexpr (std::is_arithmetic_v<T>) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << arr[i] << ",";
				}
			}
			else if constexpr (std::is_pointer_v<T>) {
				for (size_t i = 0; i < N; i++) {
					oss << " " << (arr[i] ? std::format("0x{:016X}", reinterpret_cast<std::uintptr_t>(arr[i])) : "null"s) << ",";
				}
			}
			else {
				if (SupportsGeneratedDump<T>()) {
					for (size_t i = 0; i < N; i++) {
						oss << " " << DumpGenerated(arr[i]) << ",";
					}
				}
				else {
					oss << " { unsupported } ";
				}
			}
			oss << "]";
			return oss.str();
		}

		template<typename T, size_t N, bool Primitive>
		std::string DumpArray_(const void* pArray)
		{
			(void)Primitive;
			return DumpArray_(*reinterpret_cast<const T(*)[N]>(pArray));
		}

		template<typename T, size_t N, typename F>
		std::string DumpArray_(const T(&arr)[N], F&& formatter)
		{
			std::ostringstream oss;
			oss << "[";
			for (size_t i = 0; i < N; i++) {
				oss << " " << formatter(arr[i]) << ",";
			}
			oss << "]";
			return oss.str();
		}
	}
}
