#include "Utilities.h"
#include "../log/Log.h"
#include "HrError.h"
#include <chrono>
#include <thread>
#include "objbase.h"

namespace pmon::util::win
{
	std::string GetErrorDescription(HRESULT hr) noexcept
	{
		try {
			char* descriptionWinalloc = nullptr;
			const auto result = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&descriptionWinalloc), 0, nullptr
			);

			std::string description;
			if (!result) {
				pmlog_warn("Failed formatting windows error");
			}
			else {
				description = descriptionWinalloc;
				if (LocalFree(descriptionWinalloc)) {
					pmlog_warn("Failed freeing memory for windows error formatting");
				}
				if (description.ends_with("\r\n")) {
					description.resize(description.size() - 2);
				}
			}
			return description;
		}
		catch (...) {
			pmlog_warn(ReportException("Exception in win::GetErrorDescription"));
			return {};
		}
	}

	std::string GetSEHSymbol(DWORD sehCode) noexcept
	{
#define FOR_EACH_STA(sta) case sta: return #sta
		switch (sehCode) {
			FOR_EACH_STA(STATUS_ABANDONED_WAIT_0);
			FOR_EACH_STA(STATUS_USER_APC);
			FOR_EACH_STA(STATUS_TIMEOUT);
			FOR_EACH_STA(STATUS_PENDING);
			FOR_EACH_STA(DBG_EXCEPTION_HANDLED);
			FOR_EACH_STA(DBG_CONTINUE);
			FOR_EACH_STA(STATUS_SEGMENT_NOTIFICATION);
			FOR_EACH_STA(STATUS_FATAL_APP_EXIT);
			FOR_EACH_STA(DBG_REPLY_LATER);
			FOR_EACH_STA(DBG_TERMINATE_THREAD);
			FOR_EACH_STA(DBG_TERMINATE_PROCESS);
			FOR_EACH_STA(DBG_CONTROL_C);
			FOR_EACH_STA(DBG_PRINTEXCEPTION_C);
			FOR_EACH_STA(DBG_RIPEXCEPTION);
			FOR_EACH_STA(DBG_CONTROL_BREAK);
			FOR_EACH_STA(DBG_COMMAND_EXCEPTION);
			FOR_EACH_STA(DBG_PRINTEXCEPTION_WIDE_C);
			FOR_EACH_STA(STATUS_GUARD_PAGE_VIOLATION);
			FOR_EACH_STA(STATUS_DATATYPE_MISALIGNMENT);
			FOR_EACH_STA(STATUS_BREAKPOINT);
			FOR_EACH_STA(STATUS_SINGLE_STEP);
			FOR_EACH_STA(STATUS_LONGJUMP);
			FOR_EACH_STA(STATUS_UNWIND_CONSOLIDATE);
			FOR_EACH_STA(DBG_EXCEPTION_NOT_HANDLED);
			FOR_EACH_STA(STATUS_ACCESS_VIOLATION);
			FOR_EACH_STA(STATUS_IN_PAGE_ERROR);
			FOR_EACH_STA(STATUS_INVALID_HANDLE);
			FOR_EACH_STA(STATUS_INVALID_PARAMETER);
			FOR_EACH_STA(STATUS_NO_MEMORY);
			FOR_EACH_STA(STATUS_ILLEGAL_INSTRUCTION);
			FOR_EACH_STA(STATUS_NONCONTINUABLE_EXCEPTION);
			FOR_EACH_STA(STATUS_INVALID_DISPOSITION);
			FOR_EACH_STA(STATUS_ARRAY_BOUNDS_EXCEEDED);
			FOR_EACH_STA(STATUS_FLOAT_DENORMAL_OPERAND);
			FOR_EACH_STA(STATUS_FLOAT_DIVIDE_BY_ZERO);
			FOR_EACH_STA(STATUS_FLOAT_INEXACT_RESULT);
			FOR_EACH_STA(STATUS_FLOAT_INVALID_OPERATION);
			FOR_EACH_STA(STATUS_FLOAT_OVERFLOW);
			FOR_EACH_STA(STATUS_FLOAT_STACK_CHECK);
			FOR_EACH_STA(STATUS_FLOAT_UNDERFLOW);
			FOR_EACH_STA(STATUS_INTEGER_DIVIDE_BY_ZERO);
			FOR_EACH_STA(STATUS_INTEGER_OVERFLOW);
			FOR_EACH_STA(STATUS_PRIVILEGED_INSTRUCTION);
			FOR_EACH_STA(STATUS_STACK_OVERFLOW);
			FOR_EACH_STA(STATUS_DLL_NOT_FOUND);
			FOR_EACH_STA(STATUS_ORDINAL_NOT_FOUND);
			FOR_EACH_STA(STATUS_ENTRYPOINT_NOT_FOUND);
			FOR_EACH_STA(STATUS_CONTROL_C_EXIT);
			FOR_EACH_STA(STATUS_DLL_INIT_FAILED);
			FOR_EACH_STA(STATUS_CONTROL_STACK_VIOLATION);
			FOR_EACH_STA(STATUS_FLOAT_MULTIPLE_FAULTS);
			FOR_EACH_STA(STATUS_FLOAT_MULTIPLE_TRAPS);
			FOR_EACH_STA(STATUS_REG_NAT_CONSUMPTION);
			FOR_EACH_STA(STATUS_HEAP_CORRUPTION);
			FOR_EACH_STA(STATUS_STACK_BUFFER_OVERRUN);
			FOR_EACH_STA(STATUS_INVALID_CRUNTIME_PARAMETER);
			FOR_EACH_STA(STATUS_ASSERTION_FAILURE);
			FOR_EACH_STA(STATUS_ENCLAVE_VIOLATION);
			FOR_EACH_STA(STATUS_INTERRUPTED);
			FOR_EACH_STA(STATUS_THREAD_NOT_RUNNING);
			FOR_EACH_STA(STATUS_ALREADY_REGISTERED);
			FOR_EACH_STA(STATUS_SXS_EARLY_DEACTIVATION);
			FOR_EACH_STA(STATUS_SXS_INVALID_DEACTIVATION);
		}
		return "unknown_seh_code";
#undef FOR_EACH_STA
	}

	bool WaitForNamedPipe(const std::string& fullname, int timeoutMs)
	{
		using namespace std::literals; using clock = std::chrono::high_resolution_clock;
		const auto start = clock::now();
		while (!::WaitNamedPipeA(fullname.c_str(), 10)) {
			if (clock::now() - start >= timeoutMs * 1ms) {
				return false;
			}
			std::this_thread::sleep_for(10ms);
		}
		return true;
	}

	win::Handle OpenProcess(uint32_t pid, UINT permissions)
	{
		auto hProc = (Handle)::OpenProcess(permissions, FALSE, pid);
		if (!hProc) {
			throw Except<HrError>("failed to open process");
		}
		return hProc;
	}

	std::filesystem::path GetExecutableModulePath()
	{
		char pathString[MAX_PATH];
		if (!GetModuleFileNameA(nullptr, pathString, MAX_PATH)) {
			throw Except<HrError>("failed to get this module file name");
		}
		return { pathString };
	}

	std::filesystem::path GetExecutableModulePath(HANDLE hProc)
	{
		char pathString[MAX_PATH];
		auto size = (DWORD)std::size(pathString);
		if (!QueryFullProcessImageNameA(hProc, 0, pathString, &size)) {
			throw Except<HrError>("failed to get module file name");
		}
		return { pathString };
	}

	std::filesystem::path GetExecutableModulePathFromPid(uint32_t pid)
	{
		return GetExecutableModulePath(OpenProcess(pid));
	}

	bool ProcessIs32Bit(HANDLE hProc)
	{
		BOOL isWow64 = false;
		if (IsWow64Process(hProc, &isWow64)) {
			return isWow64;
		}
		throw Except<HrError>("failed to check WOW64 status");
	}
	std::wstring GuidToString(const GUID& guid)
	{
		wchar_t buf[64];
		int len = StringFromGUID2(guid, buf, 64);
		return std::wstring(buf, len > 0 ? len - 1 : 0); // drop trailing null
	}
}