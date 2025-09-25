#include "Privileges.h"
#include "HrError.h"
#include "../log/Log.h"
#include <format>
#include "../Memory.h"

namespace pmon::util::win
{
	namespace {
		SidPtr MakeSidPtr_() {
			return SidPtr{ nullptr, &FreeSid };
		}
	}

	Handle OpenCurrentProcessToken(DWORD rightsFlags)
	{
		Handle hToken;
		if (!OpenProcessToken(GetCurrentProcess(), rightsFlags, hToken.ClearAndGetAddressOf())) {
			throw Except<HrError>("Failed opening current process token");
		}
		return hToken;
	}
	void EnablePrivilege(HANDLE hToken, const std::string& privName)
	{
		LUID luid;
		if (!LookupPrivilegeValueA(nullptr, privName.c_str(), &luid)) {
			throw Except<HrError>(std::format("Failed looking up privilege string [{}]", privName));
		}

		TOKEN_PRIVILEGES tp{ 1, {{luid, SE_PRIVILEGE_ENABLED}} };
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
		if (GetLastError() != ERROR_SUCCESS) {
			throw Except<HrError>(std::format("Failed enabling privilege [{}]", privName));
		}
	}
	TokenPack PrepareMediumIntegrityToken()
	{
		// 0) Open our token (assumedly elevated)
		auto hElevatedToken = OpenCurrentProcessToken(TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES);

		// 1) Make sure this process has the privileges to spawn a child using this token later
		EnablePrivilege(hElevatedToken, "SeIncreaseQuotaPrivilege");

		// 2) Strip admin rights & set medium IL
		Handle hRestrictedToken;
		if (!CreateRestrictedToken(
			hElevatedToken,
			LUA_TOKEN,   // UAC's built-in filter for medium IL
			0, nullptr,  // no per-SID disables
			0, nullptr,  // no priv removals
			0, nullptr,  // no extra restrict SIDs
			hRestrictedToken.ClearAndGetAddressOf()))
		{
			throw Except<HrError>("Failed creating restricted LUA token");
		}

		// 3) Duplicate into a fully-qualified primary token
		Handle hMediumDupToken;
		const DWORD rights = 
			  TOKEN_QUERY
			| TOKEN_DUPLICATE
			| TOKEN_ASSIGN_PRIMARY
			| TOKEN_ADJUST_DEFAULT
			| TOKEN_ADJUST_GROUPS
			| TOKEN_ADJUST_SESSIONID;
		if (!DuplicateTokenEx(
			hRestrictedToken,
			rights,
			nullptr,
			SecurityImpersonation,
			TokenPrimary,
			hMediumDupToken.ClearAndGetAddressOf()))
		{
			throw Except<HrError>("Failed creating restricted LUA token");
		}

		// 4) Remove High‐Mandatory group, add Medium‐Mandatory group
		SID_IDENTIFIER_AUTHORITY mAuth = SECURITY_MANDATORY_LABEL_AUTHORITY;
		// create identifiers
		auto pHighSid = MakeSidPtr_();
		if (!AllocateAndInitializeSid(&mAuth, 1, SECURITY_MANDATORY_HIGH_RID,
			0, 0, 0, 0, 0, 0, 0, OutPtr(pHighSid))) {
			throw Except<HrError>("Failed allocating high label SID");
		}
		auto pMediumSid = MakeSidPtr_();
		if (!AllocateAndInitializeSid(&mAuth, 1, SECURITY_MANDATORY_MEDIUM_RID,
			0, 0, 0, 0, 0, 0, 0, OutPtr(pMediumSid))) {
			throw Except<HrError>("Failed allocating medium label SID");
		}
		// Deny the old High label
		TOKEN_GROUPS tg{};
		tg.GroupCount = 1;
		tg.Groups[0].Sid = pHighSid.get();
		tg.Groups[0].Attributes = SE_GROUP_USE_FOR_DENY_ONLY;
		if (!AdjustTokenGroups(hMediumDupToken, FALSE, &tg, 0, nullptr, nullptr)) {
			throw Except<HrError>("Failed denying high label SID group");
		}
		// Enable the Medium label
		tg.Groups[0].Sid = pMediumSid.get();
		tg.Groups[0].Attributes = SE_GROUP_INTEGRITY | SE_GROUP_ENABLED;
		if (!AdjustTokenGroups(hMediumDupToken, FALSE, &tg, 0, nullptr, nullptr)) {
			throw Except<HrError>("Failed enabling medium label SID group");
		}

		// 5) Now set the official Mandatory Label
		TOKEN_MANDATORY_LABEL tml{};
		tml.Label.Sid = pMediumSid.get();
		tml.Label.Attributes = SE_GROUP_INTEGRITY;
		const auto tmlSize = DWORD(sizeof(tml)) + GetLengthSid(pMediumSid.get());
		if (!SetTokenInformation(hMediumDupToken, TokenIntegrityLevel, &tml, tmlSize)) {
			throw Except<HrError>("Failed setting token integrity level");
		}

		// we return this pack because pMediumSid needs to be kept alive as long as our
		// modulated token is being used
		return { std::move(pMediumSid), std::move(hMediumDupToken) };
	}
	bool WeAreElevated() noexcept
	{
		try {
			auto hToken = OpenCurrentProcessToken(TOKEN_QUERY);
			TOKEN_ELEVATION elev{}; DWORD len = 0;
			GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &len);
			return bool(elev.TokenIsElevated);
		}
		catch (...) {
			pmlog_error("Error checking elevation");
			return false;
		}
	}
}