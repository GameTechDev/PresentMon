#include "Security.h"
#include <sddl.h>
#include <vector>
#include "HrError.h"
#include "Handle.h"

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
	std::string GetProcessUserSid(uint32_t pid)
	{
		const Handle hProcess{ OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid) };
		if (!hProcess) {
			throw Except<HrError>("Failed opening process to query its user");
		}
		Handle hToken;
		if (!OpenProcessToken(hProcess, TOKEN_QUERY, hToken.ClearAndGetAddressOf())) {
			throw Except<HrError>("Failed opening process token to query its user");
		}
		DWORD size = 0;
		if (!GetTokenInformation(hToken, TokenUser, nullptr, 0, &size)
			&& GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			throw Except<HrError>("Failed sizing token user");
		}
		std::vector<std::byte> buffer(size);
		if (!GetTokenInformation(hToken, TokenUser, buffer.data(), size, &size)) {
			throw Except<HrError>("Failed reading token user");
		}
		const auto& user = *reinterpret_cast<const TOKEN_USER*>(buffer.data());
		UniqueLocalPtr<char> pSidString;
		if (!ConvertSidToStringSidA(user.User.Sid, OutPtr(pSidString))) {
			throw Except<HrError>("Failed converting user SID to string");
		}
		return pSidString.get();
	}
}