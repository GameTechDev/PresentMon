#pragma once
#include "../../CommonUtilities/log/IErrorCodeProvider.h"

namespace pwr::intel
{
	class IgclErrorCodeProvider : public pmon::util::log::IErrorCodeProvider
	{
	public:
		std::type_index GetTargetType() const override;
		pmon::util::log::IErrorCodeResolver::Strings Resolve(const pmon::util::log::ErrorCode&) const override;
	};
}
