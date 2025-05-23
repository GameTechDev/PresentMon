// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace p2c::win
{
	class ModSet
	{
	public:
		// types
		struct Descriptor
		{
			uint32_t code;
			std::string text;
		};
		// functions
		constexpr ModSet() = default;
		constexpr ModSet(uint32_t set) : set{ set } {}
		uint32_t ToPlatform() const;
		std::string GetText() const;
		bool operator==(const ModSet& rhs) const;
		bool operator<(const ModSet& rhs) const;
		bool IsSubsetOf(const ModSet& other) const;
		ModSet operator|(const ModSet& rhs) const;
		uint32_t GetCode() const;
		std::vector<std::string> GetModStrings();
		static std::optional<ModSet> FromString(const std::string& s);
		static std::optional<ModSet> SingleModFromCode(uint32_t p);
		static ModSet FromCodes(const std::vector<uint32_t>& codes);
		static std::vector<Descriptor> EnumerateMods();
	private:
		// types
		class Registry;
		// data
		uint32_t set = 0;
	};

	struct Mod
	{
		static constexpr ModSet Null{ 0 };
		static constexpr ModSet Alt{ 0b0001 };
		static constexpr ModSet Ctrl{ 0b0010 };
		static constexpr ModSet Shift{ 0b0100 };
		static constexpr ModSet Win{ 0b1000 };
	};
}