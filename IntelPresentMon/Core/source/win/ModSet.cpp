// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ModSet.h"
#include <unordered_map>

namespace p2c::win
{
	class ModSet::Registry
	{
	public:
		// functions
		Registry()
		{
			modList = {
				{ Mod::Null.GetCode(), "Null"},
				{ Mod::Alt.GetCode(), "Alt"},
				{ Mod::Ctrl.GetCode(), "Ctrl"},
				{ Mod::Shift.GetCode(), "Shift"},
				{ Mod::Win.GetCode(), "Win"},
			};
			codeMap.reserve(modList.size());
			textMap.reserve(modList.size());
			for (const auto& d : modList)
			{
				textMap[d.text] = &d;
				codeMap[d.code] = &d;
			}
		}
		const Descriptor* LookupCode(uint32_t c) const
		{
			if (auto i = codeMap.find(c); i != codeMap.end()) return i->second;
			return nullptr;
		}
		const Descriptor* LookupText(const std::string& t) const
		{
			if (auto i = textMap.find(t); i != textMap.end()) return i->second;
			return nullptr;
		}
		const std::vector<Descriptor>& EnumerateMods() const
		{
			return modList;
		}
		static const Registry& Get()
		{
			static Registry reg{};
			return reg;
		}
	private:
		std::vector<Descriptor> modList;
		std::unordered_map<uint32_t, const Descriptor*> codeMap;
		std::unordered_map<std::string, const Descriptor*> textMap;
	};




	uint32_t ModSet::ToPlatform() const
	{
		return set;
	}

	std::string ModSet::GetText() const
	{
		int count = 0;
		std::string text;
		auto set_ = set;
		uint32_t target = 0b0001;
		while (set_)
		{
			if (set_ & target)
			{
				if (auto pDesc = Registry::Get().LookupCode(target))
				{
					if (count++)
					{
						text += '+';
						text += pDesc->text;
					}
					else
					{
						text = pDesc->text;
					}
				}
				set_ -= target;
			}
			target <<= 1;
		}
		return text;
	}

	bool ModSet::operator==(const ModSet& rhs) const
	{
		return set == rhs.set;
	}

	bool ModSet::operator<(const ModSet& rhs) const
	{
		return set < rhs.set;
	}

	bool ModSet::IsSubsetOf(const ModSet& other) const
	{
		return (set & other.set) == set;
	}

	ModSet ModSet::operator|(const ModSet& rhs) const
	{
		return ModSet{ set | rhs.set };
	}

	uint32_t ModSet::GetCode() const
	{
		return set;
	}

	std::vector<std::string> ModSet::GetModStrings()
	{
		return std::vector<std::string>();
	}

	std::optional<ModSet> ModSet::FromString(const std::string& s)
	{
		if (auto pDesc = Registry::Get().LookupText(s))
		{
			return ModSet{ pDesc->code };
		}
		return {};
	}

	std::optional<ModSet> ModSet::SingleModFromCode(uint32_t p)
	{
		if (auto pDesc = Registry::Get().LookupCode(p))
		{
			return ModSet{ pDesc->code };
		}
		return {};
	}

	std::vector<ModSet::Descriptor> ModSet::EnumerateMods()
	{
		return Registry::Get().EnumerateMods();
	}

	ModSet ModSet::FromCodes(const std::vector<uint32_t>& codes)
	{
		uint32_t combined = 0;
		for (auto& code : codes) {
			combined |= code;
		}
		return { combined };
	}
}