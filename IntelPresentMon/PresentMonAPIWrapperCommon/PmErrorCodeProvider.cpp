#include "PmErrorCodeProvider.h"
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>
#include <IntelPresentMon/Interprocess/source/metadata/EnumStatus.h>
#include <IntelPresentMon/CommonUtilities/log/ErrorCode.h>
#include <IntelPresentMon/Interprocess/source/IntrospectionMacroHelpers.h>
#include "EnumMap.h"

namespace pmapi
{
	PmErrorCodeProvider::PmErrorCodeProvider()
	{
#define REG_KEY(enum_name_frag, key_name_frag, namex, short_name, descx) \
		fallbackCodeMap_[(int)PM_STATUS_ ## key_name_frag] = { \
			.symbol = STRINGIFY_MACRO_CALL(MAKE_KEY_SYMBOL(enum_name_frag, key_name_frag)), \
			.name = namex, \
			.description = descx, };
		ENUM_KEY_LIST_STATUS(REG_KEY)
#undef REG_KEY
	}
	std::type_index PmErrorCodeProvider::GetTargetType() const
	{
		return typeid(PM_STATUS);
	}
	pmon::util::log::IErrorCodeResolver::Strings PmErrorCodeProvider::Resolve(const pmon::util::log::ErrorCode& ec) const
	{
		pmon::util::log::IErrorCodeResolver::Strings strings{ .type = "PM_STATUS" };
		if (EnumMap::Initialized()) {
			auto pMap = EnumMap::GetKeyMap(PM_ENUM_STATUS);
			if (auto i = pMap->find((PM_STATUS)*ec.AsSigned()); i != pMap->end()) {
				strings.symbol = i->second.narrowSymbol;
				strings.name = i->second.narrowName;
				strings.description = i->second.narrowDescription;
			}
		}
		else if (auto i = fallbackCodeMap_.find((PM_STATUS)*ec.AsSigned()); i != fallbackCodeMap_.end()) {
			strings.symbol = i->second.symbol;
			strings.name = i->second.name;
			strings.description = i->second.description;
		}
		return strings;
	}
}