#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include <CommonUtilities/win/Handle.h>
#include <system_error>


// ties child processes to the current test case and ensures
// they are terminated regardless of how test run ends
class JobManager
{
public:
	JobManager()
		:
		hJob_{ ::CreateJobObjectA(nullptr, nullptr) }
	{
		if (!hJob_) ThrowLastError_("CreateJobObject");

		JOBOBJECT_EXTENDED_LIMIT_INFORMATION li{};
		li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		if (!::SetInformationJobObject(
			hJob_, JobObjectExtendedLimitInformation, &li, sizeof(li))) {
			ThrowLastError_("SetInformationJobObject");
		}
	}
	// Attach a child process HANDLE to the job.
	void Attach(HANDLE hChild) const
	{
		if (!::AssignProcessToJobObject(hJob_, hChild)) {
			ThrowLastError_("AssignProcessToJobObject");
		}
	}
private:
	static void ThrowLastError_(const char* where)
	{
		throw std::system_error(::GetLastError(), std::system_category(), where);
	}

	pmon::util::win::Handle hJob_;
};