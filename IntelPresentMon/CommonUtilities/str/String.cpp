// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "String.h"
#include <sstream>
#include <iomanip>
#include <ranges>
#include <cwctype>
// TODO: replace with with properly wrapped winapi include
#include <Windows.h>
#include "../log/Log.h"
#include "../Exception.h"

namespace pmon::util::str
{
	std::vector<std::string> TokenizeQuoted(const std::string& input)
	{
		std::istringstream stream;
		stream.str(input);
		std::vector<std::string> tokens;
		std::string token;

// okay to move from token and use because std::quoted(token) fills the string and does not read from
#pragma warning(push)
#pragma warning(disable : 26800)
		while (stream >> std::quoted(token))
		{
			tokens.push_back(std::move(token));
		}
#pragma warning(pop)
		return tokens;
	}

	std::wstring ToWide(const std::string& narrow) noexcept
	{
		try {
			if (narrow.empty()) {
				return {};
			}
			std::wstring wide;
			// TODO: replace with resize_and_overwrite when it becomes widely available
			wide.resize(narrow.size() + 1);
			const auto actual = MultiByteToWideChar(CP_UTF8, 0, narrow.data(), (int)narrow.size(), wide.data(), (int)wide.size());
			if (actual > 0) {
				wide.resize(actual);
				return wide;
			}
			pmlog_error("failed conversion to wide").hr();
		}
		catch (...) {
			pmlog_error(ReportException());
		}
		return {};
	}

	std::string ToNarrow(const std::wstring& wide) noexcept
	{
		try {
			if (wide.empty()) {
				return {};
			}
			std::string narrow;
			// TODO: replace with resize_and_overwrite when it becomes widely available
			narrow.resize(wide.size() * 2);
			const auto actual = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(),
				narrow.data(), (int)narrow.size(), nullptr, nullptr);
			if (actual > 0) {
				narrow.resize(actual);
				return narrow;
			}
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				const auto required = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(),
					nullptr, 0, nullptr, nullptr);
				if (required > 0) {
					narrow.resize(required);
					const auto actual = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(),
						narrow.data(), (int)narrow.size(), nullptr, nullptr);
					if (actual > 0) {
						narrow.resize(actual);
						return narrow;
					}
				}
			}
			pmlog_error("failed conversion to narrow").hr();
		}
		catch (...) {
			pmlog_error(ReportException());
		}
		return {};
	}

	template<class S>
	S TrimWhitespace(const S& input)
	{
		// Lambda to check if a character is not whitespace
		auto not_space = [](auto c) {
			if constexpr (std::same_as<S, std::wstring>) {
				return !iswspace(wint_t(c));
			}
			else {
				return !isspace(int(c));
			}
		};

		// Create a view that drops leading whitespace
		auto start = std::ranges::find_if(input, not_space);

		// Create a view of the string in reverse and drop trailing whitespace
		auto rev_end = std::ranges::find_if(input | std::views::reverse, not_space);

		// Calculate the end iterator in normal order
		auto end = rev_end.base();

		// If start is after end in the original string, return an empty string
		if (start >= end) {
			return S{};
		}

		// Create a substring from the start to the end
		return S(start, end);
	}
	template std::string TrimWhitespace<std::string>(const std::string& input);
	template std::wstring TrimWhitespace<std::wstring>(const std::wstring& input);

	std::string ToLower(const std::string& input)
	{
		return input | std::views::transform([](char c) {return (char)tolower(c); })
			| std::ranges::to<std::basic_string>();
	}

	std::wstring ToLower(const std::wstring& input)
	{
		return input | std::views::transform([](wchar_t c) {return (wchar_t)towlower(c); })
			| std::ranges::to<std::basic_string>();
	}

	std::string ToUpper(const std::string& input)
	{
		return input | std::views::transform([](char c) {return (char)toupper(c); })
			| std::ranges::to<std::basic_string>();
	}

	std::wstring ToUpper(const std::wstring& input)
	{
		return input | std::views::transform([](wchar_t c) {return (wchar_t)towupper(c); })
			| std::ranges::to<std::basic_string>();
	}
}
