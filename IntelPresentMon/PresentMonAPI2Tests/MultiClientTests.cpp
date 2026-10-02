// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "StatusComparison.h"
#include "TestProcess.h"
#include "../CommonUtilities/test/MachineExpectations.h"
#include <string>
#include <ranges>
#include <vector>
#include "Folders.h"
#include "JobManager.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace vi = std::views;
using namespace std::literals;
using namespace pmon;

namespace MultiClientTests
{
	class TestFixture : public CommonTestFixture
	{
	protected:
		const CommonProcessArgs& GetCommonArgs() const override
		{
			static CommonProcessArgs args{
				.ctrlPipe = R"(\\.\pipe\pm-multi-test-ctrl)",
				.shmNamePrefix = "pm_multi_test_intro",
				.logLevel = "debug",
				.logFolder = logFolder_,
				.sampleClientMode = "MultiClient",
			};
			return args;
		}
	};

	TEST_CLASS(CommonFixtureTests)
	{
		TestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// verify service lifetime and status command functionality
		TEST_METHOD(ServiceStatusTest)
		{
			// verify initial status
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(0ull, status.trackedPids.size());
			Assert::AreEqual(0ull, status.frameStorePids.size());
			Assert::AreEqual(16u, status.telemetryPeriodMs);
			Assert::IsFalse((bool)status.etwFlushPeriodMs);
		}
		// verify client lifetime
		TEST_METHOD(ClientLaunchTest)
		{
			auto client = fixture_.LaunchClient();
		}
		// verify client can track presenter via service
		TEST_METHOD(TrackPresenter)
		{
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			// launch client
			auto client = fixture_.LaunchClient({
				"--process-id"s, std::to_string(presenter.GetId()),
			});
		}
		// verify client can record presenter frame data
		TEST_METHOD(RecordFrames)
		{
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(150ms);
			// launch client
			auto client = fixture_.LaunchClient({
				"--process-id"s, std::to_string(presenter.GetId()),
				"--run-time"s, "1.15"s,
				"--etw-flush-period-ms"s, "8"s,
			});
			// verify frame data received
			const auto frames = std::move(client.GetFrames().frames);
			Logger::WriteMessage(std::format("Read [{}] frames\n", frames.size()).c_str());
			Assert::IsTrue(frames.size() >= 20ull, L"Minimum threshold frames received");
		}
	};

	TEST_CLASS(TelemetryPeriodTests)
	{
		TestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// basic test to see single client changing telemetry
		TEST_METHOD(OneClientSetting)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}
		}
		// two client test, 2nd client has superceded period
		TEST_METHOD(SecondClientSuperseded)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "135"s,
			});
			// check that telemetry period has not changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}
		}
		// two client test, 2nd client overrides
		TEST_METHOD(SecondClientOverrides)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "50"s,
			});
			// check that telemetry period has been overrided
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(50u, status.telemetryPeriodMs);
			}
		}
		// two client test, verify override and then reversion when clients disconnect
		TEST_METHOD(TwoClientReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "50"s,
			});
			// check that telemetry period has been overrided
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(50u, status.telemetryPeriodMs);
			}

			// kill client 2
			client2.Quit();
			// verify reversion to client 1's request
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// kill client 1
			client1.Quit();
			// verify reversion to default
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(16u, status.telemetryPeriodMs);
			}
		}
		// verify reversion on sudden client death
		TEST_METHOD(ClientMurderReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "63"s,
			});
			// check that telemetry period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "50"s,
			});
			// check that telemetry period has been overrided
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(50u, status.telemetryPeriodMs);
			}

			// murder client 2
			client2.Murder();
			// there is a lag between when a process is abruptly terminated and when the pipe ruptures
			// causing the Service session to be disposed; tolerate max 5ms
			std::this_thread::sleep_for(5ms);
			// verify reversion to client 1's request
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(63u, status.telemetryPeriodMs);
			}

			// murder client 1
			client1.Murder();
			// there is a lag between when a process is abruptly terminated and when the pipe ruptures
			// causing the Service session to be disposed; tolerate max 5ms
			std::this_thread::sleep_for(5ms);
			// verify reversion to default
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(16u, status.telemetryPeriodMs);
			}
		}
		// verify out-of-range low clamps instead of failing
		TEST_METHOD(OutOfRangeLow)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "3"s,
			});
			// check that telemetry period has been clamped
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(50u, status.telemetryPeriodMs);
		}
		// verify out-of-range high clamps instead of failing
		TEST_METHOD(OutOfRangeHigh)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--telemetry-period-ms"s, "6000"s,
			});
			// check that telemetry period has been clamped
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(5000u, status.telemetryPeriodMs);
		}
	};

	TEST_CLASS(EtwFlushPeriodTests)
	{
		TestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// basic test to see single client changing flush
		TEST_METHOD(OneClientSetting)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}
		}
		// two client test, 2nd client has superceded period
		TEST_METHOD(SecondClientSuperseded)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a client
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "65"s,
			});
			// check that flush period has not changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}
		}
		// two client test, 2nd client overrides (smaller value wins)
		TEST_METHOD(SecondClientOverrides)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a second client with a smaller period (should override)
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "35"s,
			});
			// check that flush period has been overridden
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(35u, *status.etwFlushPeriodMs);
			}
		}
		// two client test, verify override and then reversion when clients disconnect
		TEST_METHOD(TwoClientReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a second client with a smaller period (override)
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "35"s,
			});
			// verify overridden to smaller value
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(35u, *status.etwFlushPeriodMs);
			}

			// kill client 2; should revert to client 1's request
			client2.Quit();
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// kill client 1; should revert to default (manual flush disabled)
			client1.Quit();
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsFalse((bool)status.etwFlushPeriodMs);
			}
		}
		// verify reversion on sudden client death
		TEST_METHOD(ClientMurderReversion)
		{
			// launch a client
			auto client1 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "50"s,
			});
			// check that flush period has changed
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// launch a second client with a smaller period (override)
			auto client2 = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "35"s,
			});
			// verify overridden value
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(35u, *status.etwFlushPeriodMs);
			}

			// murder client 2; allow brief lag for pipe/session cleanup
			client2.Murder();
			std::this_thread::sleep_for(5ms);
			// verify reversion to client 1's request
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsTrue((bool)status.etwFlushPeriodMs);
				Assert::AreEqual(50u, *status.etwFlushPeriodMs);
			}

			// murder client 1; allow brief lag for pipe/session cleanup
			client1.Murder();
			std::this_thread::sleep_for(5ms);
			// verify reversion to default
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::IsFalse((bool)status.etwFlushPeriodMs);
			}
		}
		// verify out-of-range high clamps instead of failing
		TEST_METHOD(OutOfRangeHigh)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "1500"s,
			});
			// check that flush period has been clamped
			const auto status = fixture_.service->QueryStatus();
			Assert::IsTrue((bool)status.etwFlushPeriodMs);
			Assert::AreEqual(1000u, *status.etwFlushPeriodMs);
		}
		// verify out-of-range low clamps instead of failing
		TEST_METHOD(OutOfRangeLow)
		{
			// launch a client
			auto client = fixture_.LaunchClient({
				"--etw-flush-period-ms"s, "7"s,
			});
			// check that flush period has been clamped
			const auto status = fixture_.service->QueryStatus();
			Assert::IsTrue((bool)status.etwFlushPeriodMs);
			Assert::AreEqual(8u, *status.etwFlushPeriodMs);
		}
	};

	TEST_CLASS(TrackingTests)
	{
		TestFixture fixture_;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// verify process untrack (stream stop) when all clients close sessions
		TEST_METHOD(UntrackOnClose)
		{
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(30ms);
			// launch clients
			auto client1 = fixture_.LaunchClient({
				"--process-id"s, std::to_string(presenter.GetId()),
			});
			auto client2 = fixture_.LaunchClient({
				"--process-id"s, std::to_string(presenter.GetId()),
			});
			// verify tracking status at service
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(1ull, status.trackedPids.size());
				Assert::AreEqual(1ull, status.frameStorePids.size());
			}
			// one client quits
			client1.Quit();
			// verify tracking status at service
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(1ull, status.trackedPids.size());
				Assert::AreEqual(1ull, status.frameStorePids.size());
			}
			// other client quits
			client2.Quit();
			// verify tracking stopped at service
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(0ull, status.trackedPids.size());
				Assert::AreEqual(0ull, status.frameStorePids.size());
			}
		}
		// verify process untrack (stream stop) when clients die suddenly
		TEST_METHOD(UntrackOnMurder)
		{
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(30ms);
			// launch clients
			auto client1 = fixture_.LaunchClient({
				"--process-id"s, std::to_string(presenter.GetId()),
			});
			auto client2 = fixture_.LaunchClient({
				"--process-id"s, std::to_string(presenter.GetId()),
			});
			// verify tracking status at service
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(1ull, status.trackedPids.size());
				Assert::AreEqual(1ull, status.frameStorePids.size());
			}
			// one client dies
			client1.Murder();
			std::this_thread::sleep_for(5ms);
			// verify tracking status at service
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(1ull, status.trackedPids.size());
				Assert::AreEqual(1ull, status.frameStorePids.size());
			}
			// other client dies
			client2.Murder();
			std::this_thread::sleep_for(5ms);
			// verify tracking stopped at service
			{
				const auto status = fixture_.service->QueryStatus();
				Assert::AreEqual(0ull, status.trackedPids.size());
				Assert::AreEqual(0ull, status.frameStorePids.size());
			}
		}
		// test a large number of clients running
		TEST_METHOD(ClientStressTest)
		{
			// launch target for tracking
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(util::test::ScaleWait(150ms));
			// launch clients
			std::vector<std::unique_ptr<ClientProcess>> clientPtrs;
			for (int i = 0; i < 32; i++) {
				clientPtrs.push_back(fixture_.LaunchClientAsPtr({
					"--process-id"s, std::to_string(presenter.GetId()),
					"--run-time"s, "1.25"s,
					"--etw-flush-period-ms"s, "8"s,
				}));
			}
			// verify they all have read frames
			for (auto&&[i, pClient] : vi::enumerate(clientPtrs)) {
				const auto frames = std::move(pClient->GetFrames().frames);
				Logger::WriteMessage(std::format("Read [{}] frames from client #{}\n",
					frames.size(), i).c_str());
				Assert::IsTrue(frames.size() >= 40ull, L"Minimum threshold frames received");
			}
		}
	};

	// Every case here misuses the control pipe and then proves that a legitimate client
	// is still admitted and served.
	TEST_CLASS(TransportLifetimeTests)
	{
		TestFixture fixture_;
		static constexpr auto rejectionDeadline_ = 3s;
		// matches reservedPipeInstanceCount passed by PresentMonService::ActionServer
		static constexpr uint32_t reservedAcceptors_ = 2;

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// Connect and vanish. Repeated churn must not consume admission capacity, because
		// one accepted handle is already a whole session.
		TEST_METHOD(OneSidedConnectAndClose)
		{
			WaitForAcceptorCount_(reservedAcceptors_);
			for (int i = 0; i < 50; i++) {
				AssertRawConnects_();
				// each vanishing client costs a pipe instance, so the replacement has to
				// arrive before the next one, or the reserve erodes one connect at a time
				AssertAcceptorReserveHeld_();
			}
			AssertClientCanRunASession_();
			AssertTransportIdle_();
		}
		// Hold connections open. Holding more than the reserved instance count must not
		// block anyone, because acceptors are replenished per accept.
		TEST_METHOD(HeldConnectionsDoNotStarveAdmission)
		{
			std::vector<RawPipeClient> held;
			for (uint32_t i = 0; i < reservedAcceptors_ * 4; i++) {
				held.emplace_back(CtrlPipe_());
				AssertConnected_(held.back());
			}
			AssertClientCanRunASession_();
			held.clear();
			AssertTransportIdle_();
		}
		// guards against the suffixed names ever being reintroduced
		TEST_METHOD(HalfPipeAbuseIsNotExpressible)
		{
			AssertPipeAbsent_(CtrlPipe_() + "-in");
			AssertPipeAbsent_(CtrlPipe_() + "-out");
		}
		// a declared body larger than the protocol maximum must be refused before the
		// receive buffer is grown to hold it
		TEST_METHOD(OversizedDeclaredBodyRejected)
		{
			AssertDeclaredBodySizeRejected_(0x7fffffff);
			AssertClientCanRunASession_();
			AssertTransportIdle_();
		}
		TEST_METHOD(UndersizedDeclaredBodyRejected)
		{
			AssertDeclaredBodySizeRejected_(0);
			AssertDeclaredBodySizeRejected_(1);
			AssertClientCanRunASession_();
			AssertTransportIdle_();
		}
		// a sender that declares a length and then stalls must lose only its own session
		TEST_METHOD(PartialBodyTimesOutOnlyOffendingSession)
		{
			RawPipeClient raw{ CtrlPipe_() };
			AssertConnected_(raw);
			raw.WriteTruncatedBody(4096, 2048);
			// the victim runs for the whole duration of the offender's stall
			auto client = fixture_.LaunchClient();
			Assert::IsTrue(raw.WaitForServerClose(10s),
				L"Stalled session was not closed by the body read timeout");
			client.Quit();
			AssertTransportIdle_();
		}
		// the cap must shed load without ever becoming a permanent lockout
		TEST_METHOD(SessionCapEnforced)
		{
			// matches the SymmetricActionServer default maxConcurrentSessions
			constexpr uint32_t sessionCap = 64;
			std::vector<RawPipeClient> held;
			for (uint32_t i = 0; i < sessionCap; i++) {
				held.emplace_back(CtrlPipe_());
				AssertConnected_(held.back());
			}
			WaitForSessionCount_(sessionCap);
			RawPipeClient excess{ CtrlPipe_() };
			AssertConnected_(excess);
			Assert::IsTrue(excess.WaitForServerClose(rejectionDeadline_),
				L"Session beyond the cap was not dropped");
			held.clear();
			AssertClientCanRunASession_();
			AssertTransportIdle_();
		}
		// shutting down with traffic in every state must not hang or crash
		TEST_METHOD(ShutdownWithConnectionsInFlight)
		{
			std::vector<RawPipeClient> held;
			for (uint32_t i = 0; i < reservedAcceptors_ * 2; i++) {
				held.emplace_back(CtrlPipe_());
			}
			auto client = fixture_.LaunchClient();
			fixture_.StopService();
		}
	private:
		const std::string& CtrlPipe_() const
		{
			return fixture_.GetControlPipeName();
		}
		void AssertDeclaredBodySizeRejected_(uint32_t declaredSize)
		{
			RawPipeClient raw{ CtrlPipe_() };
			AssertConnected_(raw);
			raw.WriteDeclaredBodySize(declaredSize);
			Assert::IsTrue(raw.WaitForServerClose(rejectionDeadline_),
				L"Out of range packet body size was not rejected promptly");
		}
		void AssertRawConnects_()
		{
			RawPipeClient raw{ CtrlPipe_() };
			AssertConnected_(raw);
		}
		// a real client completing a session is the only convincing proof of serviceability
		void AssertClientCanRunASession_()
		{
			auto client = fixture_.LaunchClient();
			client.Quit();
		}
		void AssertTransportIdle_()
		{
			WaitForSessionCount_(0);
			WaitForAcceptorCount_(reservedAcceptors_);
			const auto status = fixture_.service->QueryStatus();
			Assert::AreEqual(0u, status.actionSessionCount, L"Sessions leaked after abuse");
			Assert::AreEqual(reservedAcceptors_, status.actionAcceptorCount,
				L"Acceptor pool was not replenished after abuse");
		}
		// A replacement listener is counted before the one it replaces is released, so the
		// pool can read one high for an instant but must never read low: that is exactly
		// the erosion this case is about, and it needs no settling time to observe.
		void AssertAcceptorReserveHeld_()
		{
			const auto count = fixture_.service->QueryStatus().actionAcceptorCount;
			Assert::IsTrue(count >= reservedAcceptors_, util::str::ToWide(std::format(
				"A vanishing client consumed a listener, pool down to {}", count)).c_str());
		}
		void WaitForSessionCount_(uint32_t expected)
		{
			for (int i = 0; i < 100; i++) {
				if (fixture_.service->QueryStatus().actionSessionCount == expected) {
					return;
				}
				std::this_thread::sleep_for(20ms);
			}
		}
		void WaitForAcceptorCount_(uint32_t expected)
		{
			for (int i = 0; i < 100; i++) {
				if (fixture_.service->QueryStatus().actionAcceptorCount == expected) {
					return;
				}
				std::this_thread::sleep_for(20ms);
			}
		}
		static void AssertConnected_(const RawPipeClient& raw)
		{
			Assert::IsTrue(raw.IsConnected(), util::str::ToWide(std::format(
				"Raw connect to the control pipe failed with error {}", raw.GetConnectError())).c_str());
		}
		static void AssertPipeAbsent_(const std::string& name)
		{
			const RawPipeClient raw{ name, 0ms };
			Assert::IsFalse(raw.IsConnected(), L"A suffixed half-pipe name still exists");
			Assert::AreEqual((DWORD)ERROR_FILE_NOT_FOUND, raw.GetConnectError());
		}
	};

	TEST_CLASS(ServiceCrashTests)
	{
	private:
		class Fixture_ : public CommonTestFixture
		{
		protected:
			const CommonProcessArgs& GetCommonArgs() const override
			{
				static CommonProcessArgs args{
					.ctrlPipe = R"(\\.\pipe\pm-multi-test-ctrl)",
					.shmNamePrefix = "pm_multi_test_intro",
					.logLevel = "debug",
					.logFolder = logFolder_,
					.sampleClientMode = "ServiceCrashClient",
				};
				return args;
			}
		} fixture_;
		// the case is about a client exiting at all rather than hanging on a dead service;
		// a Debug teardown (introspection, shm, query release) alone can run for seconds
		// on a loaded machine, so the budget is generous on purpose
		static constexpr auto clientExitTimeout_ = 10s;

		void RunCrashCase_(const std::vector<std::string>& args)
		{
			auto client = fixture_.LaunchClient(args);

			fixture_.StopService();

			Assert::AreEqual("exit-ack"s, client.Command("exit"));
			if (!client.WaitForExit(clientExitTimeout_)) {
				client.Murder();
				Assert::Fail(L"Client did not exit after service termination");
			}
		}

		void RunCrashCase_(pmon::test::client::CrashPhase phase)
		{
			RunCrashCase_({
				"--submode"s, std::to_string(static_cast<int>(phase)),
			});
		}

		void RunCrashCaseWithPresenter_(pmon::test::client::CrashPhase phase)
		{
			auto presenter = fixture_.LaunchPresenter();
			std::this_thread::sleep_for(30ms);

			RunCrashCase_({
				"--submode"s, std::to_string(static_cast<int>(phase)),
				"--process-id"s, std::to_string(presenter.GetId()),
			});
		}

	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			fixture_.Setup();
		}
		TEST_METHOD_CLEANUP(Cleanup)
		{
			fixture_.Cleanup();
		}
		// service drops while client has a session open
		TEST_METHOD(SessionOpen)
		{
			RunCrashCase_(pmon::test::client::CrashPhase::SessionOpen);
		}
		// service drops while client has a registered query
		TEST_METHOD(QueryRegistered)
		{
			RunCrashCase_(pmon::test::client::CrashPhase::QueryRegistered);
		}
		// service drops while client is tracking a target
		TEST_METHOD(TargetTracked)
		{
			RunCrashCaseWithPresenter_(pmon::test::client::CrashPhase::TargetTracked);
		}
		// service drops while client is polling a query/target
		TEST_METHOD(QueryPolling)
		{
			RunCrashCaseWithPresenter_(pmon::test::client::CrashPhase::QueryPolling);
		}
	};
}
