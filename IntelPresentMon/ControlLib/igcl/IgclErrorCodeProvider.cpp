#include "IgclErrorCodeProvider.h"
#include "../../CommonUtilities/log/ErrorCode.h"
#include "../../CommonUtilities/ref/GeneratedReflection.h"
#include "igcl_api.h"

using namespace pmon::util;

namespace pwr::intel
{
	std::type_index IgclErrorCodeProvider::GetTargetType() const
	{
		return typeid(ctl_result_t);
	}
	log::IErrorCodeResolver::Strings IgclErrorCodeProvider::Resolve(const pmon::util::log::ErrorCode& ec) const
	{
		return log::IErrorCodeResolver::Strings{
			.type = "ctl_result_t",
			.symbol = ref::DumpGenerated((ctl_result_t)*ec.AsSigned()),
		};
	}
}
