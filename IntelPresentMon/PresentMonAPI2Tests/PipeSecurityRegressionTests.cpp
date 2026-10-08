// Copyright (C) 2022-2025 Intel Corporation
// SPDX-License-Identifier: MIT
//
// Hermetic pipe security regressions for CI (no elevation). Full SY-listener AU squat on the
// MSI default pipe remains manual (ActionPipeProbe / optional VM).

#include "CppUnitTest.h"
#include "TestProcess.h"
#include "Folders.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonMiddleware/SharedServicePipeVerification.h"
#include "../Interprocess/source/PmStatusError.h"
#include "../PresentMonService/GlobalIdentifiers.h"
#include <format>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace as = boost::asio;
using namespace std::literals;

namespace PipeSecurityRegressionTests
{
	TEST_CLASS(PipeSecurityMask)
	{
	public:
		TEST_METHOD(ClientConnectMaskExcludesPipeCreateRights)
		{
			const DWORD mask = util::pipe::DuplexPipe::GetClientPipeConnectAccessMask();
			Assert::IsTrue((mask & FILE_CREATE_PIPE_INSTANCE) == 0);
			Assert::IsTrue((mask & WRITE_DAC) == 0);
			Assert::IsTrue((mask & WRITE_OWNER) == 0);
		}

		TEST_METHOD(ServiceControlPipeSecurityString_SyAndAuAcePresent)
		{
			const auto sddl = util::pipe::DuplexPipe::GetServiceControlPipeSecurityString();
			Assert::IsTrue(sddl.find("(A;;GA;;;SY)") != std::string::npos);
			const auto maskHex = std::format("{:x}", util::pipe::DuplexPipe::GetClientPipeConnectAccessMask());
			Assert::IsTrue(sddl.find(std::format("(A;;0x{};;;AU)", maskHex)) != std::string::npos);
		}

		TEST_METHOD(ServiceControlPipeSecurity_ClientMaskIsNotGenericReadWrite)
		{
			// Connect path must not use GENERIC_READ|GENERIC_WRITE (maps to create-instance on pipes).
			// First CreateNamedPipe caller rights are not a useful AU squat signal in CI without SY.
			const DWORD mask = util::pipe::DuplexPipe::GetClientPipeConnectAccessMask();
			Assert::AreNotEqual(GENERIC_READ | GENERIC_WRITE, mask);
		}
	};

	class PrivatePipeFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-pipe-sec-private-ctrl)",
				.shmNamePrefix = "pm_pipe_sec_private_intro",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "MultiClient",
			};
			return args;
		}
	};

	TEST_CLASS(PrivatePipeSecurityHermetic)
	{
		PrivatePipeFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}

		TEST_METHOD(PrivateControlPipe_ClientConnectOk)
		{
			Assert::IsTrue(
				util::pipe::DuplexPipe::WaitForAvailability(R"(\\.\pipe\pm-pipe-sec-private-ctrl)", 250),
				L"Private control pipe should be available for client connect");
			auto client = fixture_.LaunchClient();
			client.Quit();
		}
	};

	TEST_CLASS(SharedServicePipeVerificationLogic)
	{
	public:
		TEST_METHOD(IsDefaultSharedServiceControlPipe_NormalizesName)
		{
			Assert::IsTrue(pmon::mid::IsDefaultSharedServiceControlPipe(pmon::gid::defaultControlPipeName));
			Assert::IsTrue(pmon::mid::IsDefaultSharedServiceControlPipe("sharedpresentmonsvcnamedpipe"));
			Assert::IsFalse(pmon::mid::IsDefaultSharedServiceControlPipe(R"(\\.\pipe\pm-custom-ctrl)"));
		}

		TEST_METHOD(IsDefaultSharedServiceControlPipe_MatchesDefaultCaseInsensitively)
		{
			Assert::IsTrue(pmon::mid::IsDefaultSharedServiceControlPipe(
				R"(\\.\pipe\SHAREDPRESENTMONSVCNAMEDPIPE)"));
			Assert::IsTrue(pmon::mid::IsDefaultSharedServiceControlPipe(
				R"(\\.\PIPE\sharedpresentmonsvcnamedpipe)"));
			Assert::IsTrue(pmon::mid::IsDefaultSharedServiceControlPipe(
				"SharedPresentMonSvcNamedPipe"));
			Assert::IsFalse(pmon::mid::IsDefaultSharedServiceControlPipe(
				R"(\\.\PIPE\pm-custom-ctrl)"));
		}

		TEST_METHOD(ValidateSharedServicePipeServerProcessId_MatchSucceeds)
		{
			util::win::WindowsServiceVerificationInfo scmInfo{};
			scmInfo.processId = 4242;
			scmInfo.binaryPath = L"C:\\Program Files\\PresentMon\\PresentMonService.exe";
			scmInfo.serviceStartName = L"LocalSystem";
			pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			pmon::mid::ValidateSharedServicePipeServerProcessId(4242, scmInfo);
		}

		TEST_METHOD(ValidateSharedServicePipeServerProcessId_MismatchThrows)
		{
			util::win::WindowsServiceVerificationInfo scmInfo{};
			scmInfo.processId = 100;
			scmInfo.binaryPath = L"D:\\build\\PresentMonService.exe";
			scmInfo.serviceStartName = L"LocalSystem";
			Assert::ExpectException<pmon::ipc::PmStatusError>([&] {
				pmon::mid::ValidateSharedServicePipeServerProcessId(200, scmInfo);
			});
		}

		TEST_METHOD(ValidateSharedServiceScmRecord_RejectsBadBinary)
		{
			util::win::WindowsServiceVerificationInfo scmInfo{};
			scmInfo.processId = 1;
			scmInfo.binaryPath = L"C:\\Windows\\not-presentmon.exe";
			scmInfo.serviceStartName = L"LocalSystem";
			Assert::ExpectException<pmon::ipc::PmStatusError>([&] {
				pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			});
		}

		TEST_METHOD(ValidateSharedServiceScmRecord_AcceptsLocalSystemAliases)
		{
			util::win::WindowsServiceVerificationInfo scmInfo{};
			scmInfo.processId = 1;
			scmInfo.binaryPath = L"C:\\Program Files\\PresentMon\\PresentMonService.exe";
			scmInfo.serviceStartName = L"LocalSystem";
			pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			scmInfo.serviceStartName = L"NT AUTHORITY\\LocalSystem";
			pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			scmInfo.serviceStartName = L"nt authority\\localsystem";
			pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
		}

		TEST_METHOD(ValidateSharedServiceScmRecord_RejectsLocalSystemLookalikeAccounts)
		{
			util::win::WindowsServiceVerificationInfo scmInfo{};
			scmInfo.processId = 1;
			scmInfo.binaryPath = L"C:\\Program Files\\PresentMon\\PresentMonService.exe";
			scmInfo.serviceStartName = L".\\LocalSystem";
			Assert::ExpectException<pmon::ipc::PmStatusError>([&] {
				pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			});
			scmInfo.serviceStartName = L"DOMAIN\\LocalSystem";
			Assert::ExpectException<pmon::ipc::PmStatusError>([&] {
				pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			});
			scmInfo.serviceStartName = L"NT AUTHORITY\\LocalService";
			Assert::ExpectException<pmon::ipc::PmStatusError>([&] {
				pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			});
		}

		TEST_METHOD(ServiceExecutablePathFromCommandLine_TakesImageToken)
		{
			const auto withArgs = util::win::ServiceExecutablePathFromCommandLine(
				LR"("C:\Program Files\PresentMon\PresentMonService.exe" --log-level debug)");
			Assert::IsTrue(withArgs == LR"(C:\Program Files\PresentMon\PresentMonService.exe)");

			const auto decoy = util::win::ServiceExecutablePathFromCommandLine(
				LR"(C:\Windows\System32\cmd.exe /c C:\temp\PresentMonService.exe)");
			Assert::IsTrue(decoy == LR"(C:\Windows\System32\cmd.exe)");

			util::win::WindowsServiceVerificationInfo scmInfo{};
			scmInfo.processId = 1;
			scmInfo.serviceStartName = L"LocalSystem";
			scmInfo.binaryPath = withArgs;
			pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			scmInfo.binaryPath = decoy;
			Assert::ExpectException<pmon::ipc::PmStatusError>([&] {
				pmon::mid::ValidateSharedServiceScmRecord(scmInfo);
			});
		}
	};
}
