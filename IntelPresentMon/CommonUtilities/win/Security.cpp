#include "Security.h"
#include <sddl.h>
#include "HrError.h"

namespace pmon::util::win
{
	UniqueLocalPtr<void> MakeSecurityDescriptor(const std::string& desc)
	{
		// using <void> and not <SECURITY_DESCRIPTOR> because PSECURITY_DESCRIPTOR is void*
		UniqueLocalPtr<void> pDesc;
		if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(desc.c_str(), SDDL_REVISION_1,
			OutPtr(pDesc), nullptr)) {
			throw Except<HrError>("ConvertStringSecurityDescriptorToSecurityDescriptorA failed");
		}
		return pDesc;
	}
}