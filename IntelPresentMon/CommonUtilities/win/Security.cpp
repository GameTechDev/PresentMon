#include "Security.h"
#include "Handle.h"
#include <sddl.h>
#include "HrError.h"
#include "../Memory.h"
#include <vector>

namespace pmon::util::win
{
	std::string GetCurrentProcessUserSidString()
	{
		Handle hToken;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, hToken.ClearAndGetAddressOf())) {
			throw Except<HrError>("OpenProcessToken failed");
		}
		DWORD size = 0;
		GetTokenInformation(hToken.Get(), TokenUser, nullptr, 0, &size);
		std::vector<BYTE> buffer(size);
		if (!GetTokenInformation(hToken.Get(), TokenUser, buffer.data(), size, &size)) {
			throw Except<HrError>("GetTokenInformation TokenUser failed");
		}
		const auto* pUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
		LPSTR pSidString = nullptr;
		if (!ConvertSidToStringSidA(pUser->User.Sid, &pSidString)) {
			throw Except<HrError>("ConvertSidToStringSidA failed");
		}
		std::string sid{ pSidString };
		LocalFree(pSidString);
		return sid;
	}

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