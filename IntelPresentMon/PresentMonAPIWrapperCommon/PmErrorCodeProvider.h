#pragma once
#include <IntelPresentMon/CommonUtilities/log/IErrorCodeProvider.h>
#include <map>

namespace pmapi
{
	class PmErrorCodeProvider : public pmon::util::log::IErrorCodeProvider
	{
	public:
		PmErrorCodeProvider();
		std::type_index GetTargetType() const override;
		pmon::util::log::IErrorCodeResolver::Strings Resolve(const pmon::util::log::ErrorCode&) const override;
	private:
		std::map<uint32_t, pmon::util::log::IErrorCodeResolver::Strings> fallbackCodeMap_;
	};
}