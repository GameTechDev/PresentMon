// Copyright (C) 2022-2025 Intel Corporation
// SPDX-License-Identifier: MIT
// Exercises the action transport itself over a private pipe with toy actions, so the
// single-handle session, the packetType/commandToken demultiplexer and the session
// disposal contract can be tested without standing up the service or a CEF process.
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "../CommonUtilities/win/Handle.h"
#include "../CommonUtilities/str/String.h"
#include "../Interprocess/source/act/AsyncActionCollection.h"
#include "../Interprocess/source/act/SymmetricActionClient.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "../Interprocess/source/act/ActionServerShutdown.h"
#include <cereal/archives/binary.hpp>
#include <atomic>
#include <condition_variable>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::literals;

namespace TransportDemuxTests
{
	using namespace pmon;
	using namespace pmon::ipc::act;

	// --- server side execution context -------------------------------------------------

	struct ServerExecutionContext;

	struct ServerSessionContext
	{
		std::shared_ptr<SymmetricActionConnector<ServerExecutionContext>> pConn;
		uint32_t remotePid = 0;
	};

	struct ServerExecutionContext
	{
		using SessionContextType = ServerSessionContext;

		std::optional<uint32_t> responseWriteTimeoutMs;
		// set after the server is constructed, since an action needs the server to push
		std::function<void(uint32_t)>* pPush = nullptr;
		std::atomic<uint32_t>* pDisposeCount = nullptr;
		std::mutex* pDisposeMutex = nullptr;
		std::vector<SessionEndReason>* pDisposeReasons = nullptr;
		std::vector<SessionCleanupMode>* pDisposeModes = nullptr;
		std::atomic<uint32_t>* pUpdateTracking = nullptr;
		std::atomic<uint32_t>* pStartEtw = nullptr;
		std::vector<const char*>* pLifetimeEvents = nullptr;
		std::optional<uint32_t> bodyReadTimeoutMs;
		struct ProtocolGate
		{
			std::atomic<bool> entered{ false };
			std::atomic<bool> release{ false };
		};
		ProtocolGate* pProtocolGate = nullptr;
		struct LifetimeNote
		{
			std::vector<const char*>* events = nullptr;
			std::shared_ptr<std::atomic<bool>> borrowedAlive;
			// Assign fields after make_shared. A temporary LifetimeNote would
			// record destruction as soon as that temporary died.
			static std::shared_ptr<LifetimeNote> Make(
				std::vector<const char*>* events,
				std::shared_ptr<std::atomic<bool>> borrowedAlive)
			{
				auto note = std::make_shared<LifetimeNote>();
				note->events = events;
				note->borrowedAlive = std::move(borrowedAlive);
				return note;
			}
			~LifetimeNote()
			{
				if (borrowedAlive && !borrowedAlive->load(std::memory_order_acquire)) {
					if (events) {
						events->push_back("ExecCtxTouchedDeadBorrow");
					}
				}
				if (events) {
					events->push_back("ExecCtxDestroyed");
				}
			}
		};
		std::shared_ptr<LifetimeNote> lifetime = std::make_shared<LifetimeNote>();
		std::shared_ptr<std::atomic<SessionCleanupMode>> cleanupMode =
			std::make_shared<std::atomic<SessionCleanupMode>>(SessionCleanupMode::NormalOperation);

		void EnterFinalTeardown()
		{
			cleanupMode->store(SessionCleanupMode::FinalTeardown, std::memory_order_release);
		}
		SessionCleanupMode GetSessionCleanupMode() const
		{
			return cleanupMode->load(std::memory_order_acquire);
		}
		void Dispose(SessionContextType&, SessionDisposition disposition) const
		{
			if (pDisposeCount) {
				pDisposeCount->fetch_add(1);
			}
			if (pDisposeMutex) {
				std::lock_guard lock{ *pDisposeMutex };
				RecordDispose_(disposition);
			}
			else {
				RecordDispose_(disposition);
			}
			if (ShouldRecomputeGlobalState(disposition)) {
				if (pUpdateTracking) {
					pUpdateTracking->fetch_add(1, std::memory_order_relaxed);
				}
				if (pStartEtw) {
					pStartEtw->fetch_add(1, std::memory_order_relaxed);
				}
			}
		}
	private:
		void RecordDispose_(SessionDisposition disposition) const
		{
			if (pDisposeReasons) {
				pDisposeReasons->push_back(disposition.reason);
			}
			if (pDisposeModes) {
				pDisposeModes->push_back(disposition.cleanupMode);
			}
			if (pLifetimeEvents) {
				pLifetimeEvents->push_back("Dispose");
			}
		}
	};

	// --- client side execution context -------------------------------------------------

	struct ClientExecutionContext;

	struct ClientSessionContext
	{
		std::shared_ptr<SymmetricActionConnector<ClientExecutionContext>> pConn;
		uint32_t remotePid = 0;
	};

	struct ClientExecutionContext
	{
		using SessionContextType = ClientSessionContext;

		std::optional<uint32_t> responseWriteTimeoutMs;
		std::atomic<uint32_t>* pEventCount = nullptr;
	};

	// --- actions -----------------------------------------------------------------------

	// named to match the transport's own "first action must be OpenSession" policy
	class OpenSession : public AsyncActionBase_<OpenSession, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "OpenSession";
		struct Params
		{
			uint32_t clientPid;
			template<class A> void serialize(A& ar) { ar(clientPid); }
		};
		struct Response
		{
			uint32_t serverPid;
			template<class A> void serialize(A& ar) { ar(serverPid); }
		};
	private:
		friend class AsyncActionBase_<OpenSession, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.clientPid;
			return Response{ .serverPid = GetCurrentProcessId() };
		}
	};

	// echoes its input and, on the way, pushes pushCount unsolicited events back at the
	// caller; those events land on the same handle while this request is still outstanding
	class EchoWithPush : public AsyncActionBase_<EchoWithPush, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "EchoWithPush";
		struct Params
		{
			uint32_t value;
			uint32_t pushCount;
			template<class A> void serialize(A& ar) { ar(value, pushCount); }
		};
		struct Response
		{
			uint32_t value;
			uint32_t remotePid;
			template<class A> void serialize(A& ar) { ar(value, remotePid); }
		};
	private:
		friend class AsyncActionBase_<EchoWithPush, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext& ctx, SessionContext& stx, Params&& in)
		{
			for (uint32_t i = 0; i < in.pushCount; i++) {
				(*ctx.pPush)(in.value);
			}
			return Response{ .value = in.value, .remotePid = stx.remotePid };
		}
	};

	// Client-side traits holder; the server runs StallServer instead so a long stall
	// does not block the session reader while other requests are in flight.
	class StallForClient : public AsyncActionBase_<StallForClient, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "Stall";
		struct Params
		{
			uint32_t durationMs;
			template<class A> void serialize(A& ar) { ar(durationMs); }
		};
		struct Response
		{
			uint32_t durationMs;
			template<class A> void serialize(A& ar) { ar(durationMs); }
		};
	private:
		friend class AsyncActionBase_<StallForClient, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext&, Params&&)
		{
			assert(false && "StallForClient is client-side only");
			return Response{};
		}
	};

	// Throws from the reader coroutine. AsyncActionBase_ would swallow this as a
	// transport-failure response and the session would later end as LocalShutdown.
	class ProtocolFault : public AsyncAction<ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "ProtocolFault";
		static constexpr uint16_t Version = 1;
		struct Params
		{
			template<class A> void serialize(A& ar) {}
		};
		struct Response
		{
			template<class A> void serialize(A& ar) {}
		};
		pipe::as::awaitable<void> Execute(ServerExecutionContext& ctx, SessionContext&,
			const PacketHeader&, pipe::DuplexPipe& pipe) const override
		{
			(void)pipe.ConsumePacketPayload<Params>();
			if (ctx.pProtocolGate) {
				ctx.pProtocolGate->entered.store(true, std::memory_order_release);
				const auto deadline = std::chrono::steady_clock::now() + 2s;
				while (!ctx.pProtocolGate->release.load(std::memory_order_acquire)
					&& std::chrono::steady_clock::now() < deadline) {
					std::this_thread::sleep_for(1ms);
				}
			}
			throw util::Except<ProtocolViolation>("test protocol fault");
			co_return;
		}
		const char* GetIdentifier() const override
		{
			return Identifier;
		}
	};

	class StallServer : public AsyncAction<ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = StallForClient::Identifier;
		using Params = StallForClient::Params;
		using Response = StallForClient::Response;
		pipe::as::awaitable<void> Execute(ServerExecutionContext& ctx, SessionContext&,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const override
		{
			auto params = pipe.ConsumePacketPayload<Params>();
			pipe::as::steady_timer timer{ co_await pipe::as::this_coro::executor };
			timer.expires_after(std::chrono::milliseconds(params.durationMs));
			co_await timer.async_wait(pipe::as::use_awaitable);
			const auto resHeader = MakeResponseHeader(header, TransportStatus::Success, PM_STATUS_SUCCESS);
			co_await pipe.WritePacket(resHeader, Response{ .durationMs = params.durationMs },
				ctx.responseWriteTimeoutMs);
		}
		const char* GetIdentifier() const override
		{
			return Identifier;
		}
	};

	class PushNotice : public AsyncEventActionBase_<PushNotice, ClientExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "PushNotice";
		struct Params
		{
			uint32_t value;
			template<class A> void serialize(A& ar) { ar(value); }
		};
	private:
		friend class AsyncEventActionBase_<PushNotice, ClientExecutionContext>;
		static void Execute_(const ClientExecutionContext& ctx, SessionContext&, Params&&)
		{
			ctx.pEventCount->fetch_add(1);
		}
	};

	class DropRequest : public AsyncActionBase_<DropRequest, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "Drop";
		struct Params
		{
			template<class A> void serialize(A&) {}
		};
		struct Response
		{
			template<class A> void serialize(A&) {}
		};
	private:
		friend class AsyncActionBase_<DropRequest, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext&, Params&&)
		{
			return {};
		}
	};

	class DropServer : public AsyncAction<ServerExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ServerExecutionContext&, SessionContext&,
			const PacketHeader&, pipe::DuplexPipe& pipe) const override
		{
			pipe.ConsumePacketPayload<DropRequest::Params>();
			co_return;
		}
		const char* GetIdentifier() const override
		{
			return DropRequest::Identifier;
		}
	};

	class SpoofRequest : public AsyncActionBase_<SpoofRequest, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "Spoof";
		struct Params
		{
			uint32_t token;
			template<class A> void serialize(A& ar) { ar(token); }
		};
		struct Response
		{
			uint32_t token;
			template<class A> void serialize(A& ar) { ar(token); }
		};
	private:
		friend class AsyncActionBase_<SpoofRequest, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext&, Params&& in)
		{
			return Response{ .token = in.token };
		}
	};

	class SpoofServer : public AsyncAction<ServerExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ServerExecutionContext& ctx, SessionContext&,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const override
		{
			auto params = pipe.ConsumePacketPayload<SpoofRequest::Params>();
			const PacketHeader fake{
				.identifier = SpoofRequest::Identifier,
				.commandToken = params.token,
				.packetType = PacketType::ActionResponse,
				.headerVersion = kHeaderVersion,
			};
			co_await pipe.WritePacket(fake, EmptyPayload{}, ctx.responseWriteTimeoutMs);
			const auto resHeader = MakeResponseHeader(header, TransportStatus::Success, PM_STATUS_SUCCESS);
			co_await pipe.WritePacket(resHeader, SpoofRequest::Response{ .token = params.token }, ctx.responseWriteTimeoutMs);
		}
		const char* GetIdentifier() const override
		{
			return SpoofRequest::Identifier;
		}
	};

	class LateDouble : public AsyncActionBase_<LateDouble, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "LateDouble";
		struct Params
		{
			uint32_t value;
			template<class A> void serialize(A& ar) { ar(value); }
		};
		struct Response
		{
			uint32_t value;
			template<class A> void serialize(A& ar) { ar(value); }
		};
	private:
		friend class AsyncActionBase_<LateDouble, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext&, Params&& in)
		{
			return Response{ .value = in.value };
		}
	};

	class LateDoubleServer : public AsyncAction<ServerExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ServerExecutionContext& ctx, SessionContext&,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const override
		{
			auto params = pipe.ConsumePacketPayload<LateDouble::Params>();
			pipe::as::steady_timer timer{ co_await pipe::as::this_coro::executor };
			timer.expires_after(250ms);
			co_await timer.async_wait(pipe::as::use_awaitable);
			const auto resHeader = MakeResponseHeader(header, TransportStatus::Success, PM_STATUS_SUCCESS);
			const LateDouble::Response body{ .value = params.value };
			co_await pipe.WritePacket(resHeader, body, ctx.responseWriteTimeoutMs);
			co_await pipe.WritePacket(resHeader, body, ctx.responseWriteTimeoutMs);
		}
		const char* GetIdentifier() const override
		{
			return LateDouble::Identifier;
		}
	};

	class EchoToken : public AsyncActionBase_<EchoToken, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "EchoToken";
		struct Params
		{
			uint32_t value;
			template<class A> void serialize(A& ar) { ar(value); }
		};
		struct Response
		{
			uint32_t value;
			uint32_t token;
			template<class A> void serialize(A& ar) { ar(value, token); }
		};
	private:
		friend class AsyncActionBase_<EchoToken, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext&, Params&& in)
		{
			return Response{ .value = in.value, .token = 0 };
		}
	};

	class EchoTokenServer : public AsyncAction<ServerExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ServerExecutionContext& ctx, SessionContext&,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const override
		{
			auto params = pipe.ConsumePacketPayload<EchoToken::Params>();
			const auto resHeader = MakeResponseHeader(header, TransportStatus::Success, PM_STATUS_SUCCESS);
			co_await pipe.WritePacket(resHeader, EchoToken::Response{
				.value = params.value, .token = header.commandToken }, ctx.responseWriteTimeoutMs);
		}
		const char* GetIdentifier() const override
		{
			return EchoToken::Identifier;
		}
	};
}

namespace pmon::ipc::act
{
	template<> struct ActionParamsTraits<TransportDemuxTests::OpenSession::Params>
	{
		using Action = TransportDemuxTests::OpenSession;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::EchoWithPush::Params>
	{
		using Action = TransportDemuxTests::EchoWithPush;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::StallForClient::Params>
	{
		using Action = TransportDemuxTests::StallForClient;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::ProtocolFault::Params>
	{
		using Action = TransportDemuxTests::ProtocolFault;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::PushNotice::Params>
	{
		using Action = TransportDemuxTests::PushNotice;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::DropRequest::Params>
	{
		using Action = TransportDemuxTests::DropRequest;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::SpoofRequest::Params>
	{
		using Action = TransportDemuxTests::SpoofRequest;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::LateDouble::Params>
	{
		using Action = TransportDemuxTests::LateDouble;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::EchoToken::Params>
	{
		using Action = TransportDemuxTests::EchoToken;
	};
}

namespace TransportDemuxTests
{
	void RegisterActions_()
	{
		static std::once_flag once;
		std::call_once(once, [] {
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<OpenSession>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<EchoWithPush>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<StallServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<ProtocolFault>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<DropServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<SpoofServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<LateDoubleServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<EchoTokenServer>());
			AsyncActionCollection<ClientExecutionContext>::Get().AddAction<PushNotice>();
		});
	}

	struct ServerTestHooks
	{
		std::vector<SessionCleanupMode>* disposeModes = nullptr;
		std::atomic<uint32_t>* updateTracking = nullptr;
		std::atomic<uint32_t>* startEtw = nullptr;
		std::vector<const char*>* events = nullptr;
		std::optional<uint32_t> bodyReadTimeoutMs;
		ServerExecutionContext::ProtocolGate* protocolGate = nullptr;
		std::shared_ptr<std::atomic<bool>> borrowedAlive;
	};

	// Policy selects single-peer push, request/response, or addressed send.
	template<class Policy = SinglePeerServerPolicy>
	class TestServer
	{
	public:
		static constexpr uint32_t reservedAcceptors = 2;
		TestServer(const std::string& pipeName, std::atomic<uint32_t>& disposeCount,
			std::mutex* disposeMutex = nullptr, std::vector<SessionEndReason>* disposeReasons = nullptr,
			ServerTestHooks hooks = {})
			:
			server_{ ServerExecutionContext{
					.pPush = &push_,
					.pDisposeCount = &disposeCount,
					.pDisposeMutex = disposeMutex,
					.pDisposeReasons = disposeReasons,
					.pDisposeModes = hooks.disposeModes,
					.pUpdateTracking = hooks.updateTracking,
					.pStartEtw = hooks.startEtw,
					.pLifetimeEvents = hooks.events,
					.bodyReadTimeoutMs = hooks.bodyReadTimeoutMs,
					.pProtocolGate = hooks.protocolGate,
					.lifetime = ServerExecutionContext::LifetimeNote::Make(
						hooks.events, hooks.borrowedAlive),
				},
				pipeName, reservedAcceptors, std::string{} }
		{
			push_ = [this](uint32_t value) {
				if constexpr (Policy::kUnaddressedSend) {
					server_.DispatchDetached(PushNotice::Params{ .value = value });
				}
				else {
					(void)value;
				}
			};
		}
		uint32_t GetSessionCount() const { return server_.GetSessionCount(); }
		uint32_t GetAcceptorCount() const { return server_.GetAcceptorCount(); }
		LifecyclePhase GetLifecyclePhase() const { return server_.GetLifecyclePhase(); }
		void EnterFinalTeardown() { server_.EnterFinalTeardown(); }
		void BeginShutdown() { server_.BeginShutdown(); }
		bool WaitForShutdown() { return server_.WaitForShutdown(); }
		bool IsRunning() const { return server_.IsRunning(); }
		template<class Params>
		void DispatchDetached(Params&& params)
		{
			server_.DispatchDetached(std::forward<Params>(params));
		}
		std::vector<uint32_t> CopySessionIds() { return server_.CopySessionIds(); }
		template<class Params>
		void DispatchToSession(uint32_t sessionId, Params&& params)
		{
			server_.DispatchDetached(sessionId, std::forward<Params>(params));
		}
	private:
		std::function<void(uint32_t)> push_;
		SymmetricActionServer<ServerExecutionContext, Policy> server_;
	};

	class TestClient : public SymmetricActionClient<ClientExecutionContext>
	{
	public:
		TestClient(const std::string& pipeName, std::atomic<uint32_t>& eventCount)
			:
			SymmetricActionClient{ pipeName, ClientExecutionContext{ .pEventCount = &eventCount } }
		{
			const auto res = DispatchSync(OpenSession::Params{ .clientPid = GetCurrentProcessId() });
			EstablishSession_(res.serverPid);
		}
	};

	std::string MakeUniquePipeName_()
	{
		static std::atomic<uint32_t> nextId = 0;
		return std::format(R"(\\.\pipe\pm-demux-test-{}-{})", GetCurrentProcessId(), nextId++);
	}

	struct AdmissionHookState_
	{
		TestClient* client = nullptr;
		std::atomic<bool> sawRunning{ false };
		std::mutex mutex;
		std::condition_variable cv;
		bool entered = false;
		bool release = false;
	};

	inline AdmissionHookState_* gAdmissionHook_ = nullptr;

	inline void AdmissionHookFn_()
	{
		auto* hook = gAdmissionHook_;
		if (!hook) {
			return;
		}
		hook->sawRunning.store(hook->client
			&& hook->client->GetLifecyclePhase() == LifecyclePhase::Running);
		std::unique_lock lock{ hook->mutex };
		hook->entered = true;
		hook->cv.notify_all();
		hook->cv.wait(lock, [&] { return hook->release; });
	}

	std::unique_ptr<TestClient> ConnectClient_(const std::string& pipeName, std::atomic<uint32_t>& eventCount)
	{
		Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000),
			L"Timed out waiting for the test transport pipe");
		return std::make_unique<TestClient>(pipeName, eventCount);
	}

	TEST_CLASS(DemuxTests)
	{
	public:
		TEST_METHOD_INITIALIZE(Setup)
		{
			RegisterActions_();
		}
		// the framing floor must match what the protocol actually serializes, otherwise
		// the pipe layer rejects legitimate minimal packets or lets junk through
		TEST_METHOD(MinimalPacketBodySizeMatchesProtocolFloor)
		{
			std::ostringstream body;
			{
				cereal::BinaryOutputArchive archive{ body };
				archive(PacketHeader{}, EmptyPayload{});
			}
			Assert::AreEqual((size_t)kMinPacketBodyBytes, body.str().size(),
				L"kMinPacketBodyBytes no longer matches the serialized size of an empty header");
		}
		// the whole point of the demux: an event pushed while a request is in flight must
		// not be mistaken for that request's response
		TEST_METHOD(PushDuringRequestResolvesCorrectResponse)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			constexpr uint32_t requestCount = 200;
			constexpr uint32_t pushesPerRequest = 3;
			for (uint32_t i = 0; i < requestCount; i++) {
				const auto res = pClient->DispatchSync(EchoWithPush::Params{
					.value = i, .pushCount = pushesPerRequest });
				Assert::AreEqual(i, res.value, L"Response did not match its own request");
				Assert::AreEqual((uint32_t)GetCurrentProcessId(), res.remotePid);
			}
			// pushes are detached, so allow them to drain before counting
			for (int i = 0; i < 100 && eventCount.load() < requestCount * pushesPerRequest; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(requestCount * pushesPerRequest, eventCount.load(),
				L"Pushed events were lost while requests were in flight");
		}
		// concurrent callers on one session share a reader loop; each must get its own answer
		TEST_METHOD(ConcurrentRequestsOnOneSessionDoNotCrossTalk)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			constexpr uint32_t threadCount = 4;
			constexpr uint32_t perThread = 50;
			std::atomic<uint32_t> mismatches = 0;
			std::vector<std::jthread> threads;
			for (uint32_t t = 0; t < threadCount; t++) {
				threads.emplace_back([&, t] {
					for (uint32_t i = 0; i < perThread; i++) {
						const auto value = t * 1000 + i;
						const auto res = pClient->DispatchSync(EchoWithPush::Params{
							.value = value, .pushCount = 1 });
						if (res.value != value) {
							mismatches.fetch_add(1);
						}
					}
				});
			}
			threads.clear();
			Assert::AreEqual(0u, mismatches.load(), L"A response was delivered to the wrong caller");
		}
		// two sessions from one process exercise the per-session pending-response maps,
		// where the two token counters are free to collide in value
		TEST_METHOD(MultipleSessionsFromOneProcess)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCountA = 0;
			std::atomic<uint32_t> eventCountB = 0;
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			auto pClientA = ConnectClient_(pipeName, eventCountA);
			auto pClientB = ConnectClient_(pipeName, eventCountB);
			Assert::AreEqual(2u, server.GetSessionCount());

			for (uint32_t i = 0; i < 50; i++) {
				const auto resA = pClientA->DispatchSync(EchoWithPush::Params{ .value = i, .pushCount = 0 });
				const auto resB = pClientB->DispatchSync(EchoWithPush::Params{ .value = i + 500, .pushCount = 0 });
				Assert::AreEqual(i, resA.value);
				Assert::AreEqual(i + 500, resB.value);
			}
		}
		// an accept immediately yields a session and a replacement acceptor, so churn
		// cannot erode the pool the way unpaired half-connections used to
		TEST_METHOD(AcceptorPoolSurvivesConnectChurn)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };

			for (int i = 0; i < 25; i++) {
				auto pClient = ConnectClient_(pipeName, eventCount);
				const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 7, .pushCount = 0 });
				Assert::AreEqual(7u, res.value);
			}
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(0u, server.GetSessionCount());
			Assert::AreEqual(25u, disposeCount.load(), L"A disposed session was not cleaned up");
		}
		TEST_METHOD(ClientIsNotRunningAfterServerDisconnect)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::unique_ptr<TestClient> pClient;
			{
				TestServer server{ pipeName, disposeCount };
				pClient = ConnectClient_(pipeName, eventCount);
				Assert::IsTrue(pClient->IsRunning(), L"Client should be running while session is active");
			}
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(),
				L"Client io context should stop when the session reader strand exits");
		}
		// regression for the dispose guard: cleanup used to be skipped entirely when a
		// session never reported a remote pid
		TEST_METHOD(DisposeRunsForSessionThatNeverOpened)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			{
				Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
				SymmetricActionClient<ClientExecutionContext> bareClient{
					pipeName, ClientExecutionContext{ .pEventCount = &eventCount } };
				WaitForSessionCount_(server, 1);
			}
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(1u, disposeCount.load(),
				L"Dispose did not run for a session that never sent OpenSession");
		}
		// The listener count can be observed directly: every client that connects and
		// vanishes costs one pipe instance, and every one of them must be replaced or
		// the server eventually has nothing left to accept on
		TEST_METHOD(ConnectAndCloseChurnKeepsListenerReserve)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			// Multi-peer: a vanished client can still be in the map for a moment, and
			// that must not keep the next real client from being admitted.
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			WaitForAcceptorCount_(server, TestServer<>::reservedAcceptors);

			for (int i = 0; i < 50; i++) {
				RawConnectAndClose_(pipeName);
				// a replacement is counted before the listener it replaces is released, so
				// the pool can read one high for an instant but must never read low
				Assert::IsTrue(server.GetAcceptorCount() >= TestServer<>::reservedAcceptors,
					L"A vanishing client consumed a listener from the pool");
			}
			WaitForAcceptorCount_(server, TestServer<>::reservedAcceptors);
			Assert::AreEqual(TestServer<>::reservedAcceptors, server.GetAcceptorCount(),
				L"Listener pool did not settle back to its reserve");
			// the pool is not just counted correctly, it still admits a real client
			auto pClient = ConnectClient_(pipeName, eventCount);
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 7, .pushCount = 0 });
			Assert::AreEqual(7u, res.value);
		}
		// A continuation shuts down its own client while another request on that
		// client is still pending. Ownership stays with this test until both settle.
		TEST_METHOD(PendingRequestFailsWhenShutdownFromContinuationOnIoThread)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			std::atomic<bool> pendingFailed = false;
			std::atomic<bool> pendingFinished = false;
			std::jthread pendingRequester{ [&] {
				try {
					pClient->DispatchSync(DropRequest::Params{});
				}
				catch (...) {
					pendingFailed = true;
				}
				pendingFinished = true;
			} };
			std::this_thread::sleep_for(100ms);

			std::atomic<int> continuationCalls = 0;
			std::atomic<bool> shutdownPosted = false;
			pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 1, .pushCount = 0 },
				[&](EchoWithPush::Response&&, std::exception_ptr) {
					continuationCalls.fetch_add(1);
					pClient->BeginShutdown();
					shutdownPosted = true;
				});

			for (int i = 0; i < 300 && (!pendingFinished || !shutdownPosted); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1, continuationCalls.load(), L"Continuation did not run exactly once");
			Assert::IsTrue(shutdownPosted.load(), L"Shutdown was not initiated from the continuation");
			Assert::IsTrue(pendingFinished.load(), L"Same-client shutdown left a pending request hung");
			Assert::IsTrue(pendingFailed.load(), L"A pending request during same-client shutdown did not fail");
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(PendingRequestFailsWhenClientShutsDown)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			std::atomic<bool> failed = false;
			std::atomic<bool> finished = false;
			std::jthread requester{ [&] {
				try {
					pClient->DispatchSync(DropRequest::Params{});
				}
				catch (...) {
					failed = true;
				}
				finished = true;
			} };
			// let the responder consume the request without answering it
			std::this_thread::sleep_for(100ms);
			pClient->BeginShutdown();
			for (int i = 0; i < 300 && !finished; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsTrue(finished.load(), L"Shutdown left a request waiting for a response");
			Assert::IsTrue(failed.load(), L"A request that can never be answered reported success");
		}
		// shutdown cleanup must not become a stall: a client whose server is already gone,
		// and one whose server is still there, both have to tear down promptly
		TEST_METHOD(ClientShutdownIsPrompt)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			{
				TestServer server{ pipeName, disposeCount };
				auto pClient = ConnectClient_(pipeName, eventCount);
				AssertTeardownIsPrompt_(pClient, L"with a live server");
				WaitForSessionCount_(server, 0);
				Assert::AreEqual(1u, disposeCount.load(), L"Live server did not dispose the torn-down client");
				Assert::AreEqual(TestServer<>::reservedAcceptors, server.GetAcceptorCount());
			}
			AssertPipeAbsent_(pipeName);
			std::unique_ptr<TestClient> pOrphan;
			{
				TestServer server{ pipeName, disposeCount };
				try {
					pOrphan = ConnectClient_(pipeName, eventCount);
				}
				catch (const std::exception& ex) {
					Assert::Fail(util::str::ToWide(std::format(
						"second connect failed sessions={} acceptors={} what={}",
						server.GetSessionCount(), server.GetAcceptorCount(), ex.what())).c_str());
				}
			}
			AssertTeardownIsPrompt_(pOrphan, L"after the server dropped");
		}
		// a responder that goes quiet must cost the requester a bounded wait, and only
		// that request: the session itself stays usable
		TEST_METHOD(ResponseTimeoutFailsOnlyTheRequest)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);

			Assert::ExpectException<ResponseTimeout>([&] {
				pClient->DispatchSync(StallForClient::Params{ .durationMs = 1000 }, 100);
			}, L"A request that was never answered did not time out");
			// the abandoned response arrives late and is dropped, leaving the stream usable
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 11, .pushCount = 0 });
			Assert::AreEqual(11u, res.value, L"A timed out request poisoned its session");
			Assert::AreEqual(1u, server.GetSessionCount(), L"A timed out request dropped its session");
		}
		// the suffixed names are the shape of the reported defect; they must not come back
		TEST_METHOD(HalfPipeNamesAreNotExpressible)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			AssertPipeAbsent_(pipeName + "-in");
			AssertPipeAbsent_(pipeName + "-out");
		}
		TEST_METHOD(RepeatedShutdownIsIdempotent)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			pClient->BeginShutdown();
			pClient->BeginShutdown();
			pClient->WaitForShutdown();
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(DestructionAfterAsyncShutdownIsPrompt)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			pClient->BeginShutdown();
			const auto start = std::chrono::steady_clock::now();
			pClient.reset();
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"Destroying a client after BeginShutdown stalled");
		}
		TEST_METHOD(ShutdownAfterPeerDisconnectReachesStopped)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			std::unique_ptr<TestClient> pClient;
			{
				TestServer server{ pipeName, disposeCount };
				pClient = ConnectClient_(pipeName, eventCount);
			}
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning());
			pClient->BeginShutdown();
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(DispatchRacingShutdownSettlesOrRejects)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			std::atomic<uint32_t> completed = 0;
			std::atomic<uint32_t> rejected = 0;
			std::vector<std::jthread> threads;
			for (int t = 0; t < 4; t++) {
				threads.emplace_back([&] {
					const auto deadline = std::chrono::steady_clock::now() + 3s;
					while (std::chrono::steady_clock::now() < deadline) {
						try {
							const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 3, .pushCount = 0 });
							if (res.value == 3) {
								completed.fetch_add(1);
							}
						}
						catch (...) {
							rejected.fetch_add(1);
							return;
						}
					}
				});
			}
			std::this_thread::sleep_for(30ms);
			pClient->BeginShutdown();
			threads.clear();
			Assert::IsTrue(rejected.load() > 0, L"Shutdown never rejected or failed a dispatch");
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
		}
		TEST_METHOD(ContinuationRunsExactlyOnce)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			std::atomic<int> calls = 0;
			pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 9, .pushCount = 0 },
				[&](EchoWithPush::Response&& res, std::exception_ptr error) {
					calls.fetch_add(1);
					Assert::IsTrue(error == nullptr);
					Assert::AreEqual(9u, res.value);
				});
			for (int i = 0; i < 200 && calls.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			std::this_thread::sleep_for(50ms);
			Assert::AreEqual(1, calls.load());
		}
		TEST_METHOD(DisposeOnceOnDisconnectProtocolFailureAndLocalShutdown)
		{
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::atomic<uint32_t> eventCount = 0;
				TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount, &disposeMutex, &reasons };
				{
					auto pClient = ConnectClient_(pipeName, eventCount);
					WaitForSessionCount_(server, 1);
				}
				WaitForSessionCount_(server, 0);
				Assert::AreEqual(1u, disposeCount.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected);
			}
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::atomic<uint32_t> eventCountA = 0;
				std::atomic<uint32_t> eventCountB = 0;
				TestServer<RequestResponseServerPolicy> server{ pipeName, disposeCount, &disposeMutex, &reasons };
				auto pOther = ConnectClient_(pipeName, eventCountB);
				(void)eventCountA;
				std::ostringstream body;
				{
					cereal::BinaryOutputArchive archive{ body };
					PacketHeader header{
						.identifier = "Nope",
						.commandToken = 0xA11F0000u,
						.packetType = PacketType::ActionResponse,
						.headerVersion = kHeaderVersion,
					};
					archive(header, EmptyPayload{});
				}
				const auto payload = body.str();
				const uint32_t payloadSize = (uint32_t)payload.size();
				util::win::Handle raw;
				for (int i = 0; i < 200 && !raw; i++) {
					raw = util::win::Handle{ CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
						0, nullptr, OPEN_EXISTING, 0, nullptr) };
					if (!raw) {
						std::this_thread::sleep_for(5ms);
					}
				}
				Assert::IsTrue((bool)raw, L"Could not open a raw session to inject an unknown response");
				DWORD written = 0;
				Assert::IsTrue(WriteFile(raw, &payloadSize, sizeof(payloadSize), &written, nullptr) != FALSE);
				Assert::IsTrue(WriteFile(raw, payload.data(), (DWORD)payload.size(), &written, nullptr) != FALSE);
				for (int i = 0; i < 200 && disposeCount.load() == 0; i++) {
					std::this_thread::sleep_for(10ms);
				}
				Assert::AreEqual(1u, server.GetSessionCount());
				const auto res = pOther->DispatchSync(EchoWithPush::Params{ .value = 4, .pushCount = 0 });
				Assert::AreEqual(4u, res.value);
				Assert::AreEqual(1u, disposeCount.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::ProtocolFailure);
			}
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::atomic<uint32_t> eventCount = 0;
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons };
				auto pClient = ConnectClient_(pipeName, eventCount);
				WaitForSessionCount_(server, 1);
				server.BeginShutdown();
				server.WaitForShutdown();
				Assert::IsTrue(server.GetLifecyclePhase() == LifecyclePhase::Stopped);
				Assert::AreEqual(1u, disposeCount.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::LocalShutdown);
			}
		}
		TEST_METHOD(DuplicateResponseAfterLateResponseKillsSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons };
			auto pClient = ConnectClient_(pipeName, eventCount);
			Assert::ExpectException<ResponseTimeout>([&] {
				pClient->DispatchSync(LateDouble::Params{ .value = 5 }, 40);
			});
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(), L"A second response for a consumed expired token left the session up");
			WaitForSessionCount_(server, 0);
			Assert::AreEqual(1u, disposeCount.load());
		}
		TEST_METHOD(ExpirationEvictionTreatsForgottenTokenAsUnknown)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			for (size_t i = 0; i < kMaxExpiredResponseTokens + 1; i++) {
				Assert::ExpectException<ResponseTimeout>([&] {
					pClient->DispatchSync(DropRequest::Params{}, 30);
				});
			}
			const auto kept = pClient->DispatchSync(SpoofRequest::Params{ .token = 2 });
			Assert::AreEqual(2u, kept.token);
			Assert::IsTrue(pClient->IsRunning());
			try {
				pClient->DispatchSync(SpoofRequest::Params{ .token = 1 });
			}
			catch (...) {
			}
			for (int i = 0; i < 200 && pClient->IsRunning(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsFalse(pClient->IsRunning(), L"A response for an evicted token did not end the session");
		}
		TEST_METHOD(CommandTokenAllocatorDefaultStartsAtZero)
		{
			CommandTokenAllocator allocator;
			const auto unavailable = [](uint32_t) { return false; };
			Assert::AreEqual(0u, allocator.Allocate(unavailable));
			Assert::AreEqual(1u, allocator.Allocate(unavailable));
		}
		TEST_METHOD(CommandTokenAllocatorSkipsUnavailableNearWrap)
		{
			CommandTokenAllocator allocator{ 0xFFFFFFFFu };
			const std::unordered_set<uint32_t> pending{ 0 };
			const std::unordered_set<uint32_t> expired{ 1 };
			const auto unavailable = [&](uint32_t token) {
				return pending.contains(token) || expired.contains(token);
			};
			Assert::AreEqual(0xFFFFFFFFu, allocator.Allocate(unavailable));
			Assert::AreEqual(2u, allocator.Allocate(unavailable));
		}
		TEST_METHOD(LiveTokenAllocationSkipsPendingAndExpiredOnWrap)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			class ProbeClient : public SymmetricActionClient<ClientExecutionContext>
			{
			public:
				ProbeClient(const std::string& pipe, std::atomic<uint32_t>& events, CommandTokenAllocator tokens)
					:
					SymmetricActionClient{ pipe, ClientExecutionContext{ .pEventCount = &events }, std::move(tokens) }
				{
					const auto res = DispatchSync(OpenSession::Params{ .clientPid = GetCurrentProcessId() });
					EstablishSession_(res.serverPid);
				}
				void Probe(std::atomic<uint32_t>& skippedPending, std::atomic<uint32_t>& skippedExpired, std::atomic<int>& done)
				{
					DispatchWithContinuation(EchoWithPush::Params{ .value = 1, .pushCount = 0 },
						[this, &skippedPending, &skippedExpired, &done](EchoWithPush::Response&&, std::exception_ptr error) {
							if (!error) {
								struct FakePending : PendingResponse
								{
									void OnComplete_(const PacketHeader&, util::pipe::DuplexPipe&) override {}
									void OnFail_(std::exception_ptr) override {}
								} fake;
								SessionConnector_().ExpirePending(0);
								skippedExpired = SessionConnector_().AllocateCommandToken();
								SessionConnector_().RegisterPending(2, fake);
								skippedPending = SessionConnector_().AllocateCommandToken();
								SessionConnector_().ExpirePending(2);
							}
							done.fetch_add(1);
						});
				}
			};
			ProbeClient client{ pipeName, eventCount, CommandTokenAllocator{ 0xFFFFFFFEu } };
			std::atomic<uint32_t> skippedPending = 0;
			std::atomic<uint32_t> skippedExpired = 0;
			std::atomic<int> done = 0;
			client.Probe(skippedPending, skippedExpired, done);
			for (int i = 0; i < 200 && done.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::AreEqual(1, done.load());
			Assert::AreEqual(1u, skippedExpired.load(), L"Wrap landed on a retained expiration");
			Assert::AreEqual(3u, skippedPending.load(), L"Wrap landed on a pending token");
		}
		TEST_METHOD(LiveRequestsWrapCommandTokens)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			class WrapClient : public SymmetricActionClient<ClientExecutionContext>
			{
			public:
				WrapClient(const std::string& pipe, std::atomic<uint32_t>& events)
					:
					SymmetricActionClient{ pipe, ClientExecutionContext{ .pEventCount = &events },
						CommandTokenAllocator{ 0xFFFFFFFEu } }
				{
					const auto res = DispatchSync(OpenSession::Params{ .clientPid = GetCurrentProcessId() });
					EstablishSession_(res.serverPid);
				}
			};
			WrapClient client{ pipeName, eventCount };
			const auto first = client.DispatchSync(EchoToken::Params{ .value = 10 });
			const auto second = client.DispatchSync(EchoToken::Params{ .value = 11 });
			const auto third = client.DispatchSync(EchoToken::Params{ .value = 12 });
			Assert::AreEqual(10u, first.value);
			Assert::AreEqual(0xFFFFFFFFu, first.token);
			Assert::AreEqual(11u, second.value);
			Assert::AreEqual(0u, second.token);
			Assert::AreEqual(12u, third.value);
			Assert::AreEqual(1u, third.token);
		}
		TEST_METHOD(SinglePeerRejectsSecondSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			bool refused = false;
			try {
				std::atomic<uint32_t> extraEvents = 0;
				auto pExtra = ConnectClient_(pipeName, extraEvents);
				for (int i = 0; i < 200 && pExtra->IsRunning(); i++) {
					std::this_thread::sleep_for(10ms);
				}
				refused = !pExtra->IsRunning();
			}
			catch (...) {
				refused = true;
			}
			Assert::IsTrue(refused, L"Single-peer server accepted a second session");
			Assert::AreEqual(1u, server.GetSessionCount());
			const auto res = pClient->DispatchSync(EchoWithPush::Params{ .value = 6, .pushCount = 0 });
			Assert::AreEqual(6u, res.value);
		}
		TEST_METHOD(AddressedSendRequiresExplicitSession)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCountA = 0;
			std::atomic<uint32_t> eventCountB = 0;
			TestServer<AddressedMultiPeerServerPolicy> server{ pipeName, disposeCount };
			Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
			TestClient clientA{ pipeName, eventCountA };
			TestClient clientB{ pipeName, eventCountB };
			const auto ids = server.CopySessionIds();
			Assert::AreEqual((size_t)2, ids.size());
			Assert::ExpectException<util::Exception>([&] {
				server.DispatchToSession(0xFFFFFFFFu, PushNotice::Params{ .value = 1 });
			});
			server.DispatchToSession(ids[0], PushNotice::Params{ .value = 2 });
			for (int i = 0; i < 200 && eventCountA.load() + eventCountB.load() == 0; i++) {
				std::this_thread::sleep_for(10ms);
			}
			std::this_thread::sleep_for(50ms);
			Assert::AreEqual(1u, eventCountA.load() + eventCountB.load());
			Assert::IsTrue(eventCountA.load() != eventCountB.load());
		}
		TEST_METHOD(DispatchAcceptanceIsDecidedWithScheduling)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			AdmissionHookState_ hook;
			hook.client = pClient.get();
			gAdmissionHook_ = &hook;
			pClient->SetAdmissionHookForTest(&AdmissionHookFn_);
			std::atomic<bool> dispatchDone{ false };
			std::jthread dispatcher{ [&] {
				try {
					(void)pClient->DispatchSync(EchoWithPush::Params{ .value = 4, .pushCount = 0 });
				}
				catch (...) {
				}
				dispatchDone.store(true);
			} };
			{
				std::unique_lock lock{ hook.mutex };
				Assert::IsTrue(hook.cv.wait_for(lock, 2s, [&] { return hook.entered; }),
					L"Dispatch never entered the admission gate");
			}
			Assert::IsTrue(hook.sawRunning.load(), L"Admission hook ran after the phase left Running");
			std::jthread shutdown{ [&] { pClient->BeginShutdown(); } };
			std::this_thread::sleep_for(50ms);
			{
				std::lock_guard lock{ hook.mutex };
				hook.release = true;
			}
			hook.cv.notify_all();
			dispatcher.join();
			shutdown.join();
			pClient->SetAdmissionHookForTest(nullptr);
			gAdmissionHook_ = nullptr;
			Assert::IsTrue(dispatchDone.load(), L"Accepted dispatch did not finish");
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
			std::atomic<int> calls = 0;
			Assert::ExpectException<ServerDroppedError>([&] {
				pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 5, .pushCount = 0 },
					[&](EchoWithPush::Response&&, std::exception_ptr) { calls.fetch_add(1); });
			});
			std::this_thread::sleep_for(30ms);
			Assert::AreEqual(0, calls.load(), L"Rejected continuation was invoked");
		}
		TEST_METHOD(ReaderExitAdmissionIsDecidedWithScheduling)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			AdmissionHookState_ hook;
			hook.client = pClient.get();
			gAdmissionHook_ = &hook;
			pClient->SetAdmissionHookForTest(&AdmissionHookFn_);
			std::atomic<bool> dispatchDone{ false };
			std::jthread dispatcher{ [&] {
				try {
					(void)pClient->DispatchSync(EchoWithPush::Params{ .value = 4, .pushCount = 0 });
				}
				catch (...) {
				}
				dispatchDone.store(true);
			} };
			{
				std::unique_lock lock{ hook.mutex };
				Assert::IsTrue(hook.cv.wait_for(lock, 2s, [&] { return hook.entered; }),
					L"Dispatch never entered the admission gate");
			}
			Assert::IsTrue(hook.sawRunning.load(), L"Admission hook ran after the phase left Running");
			server.BeginShutdown();
			for (int i = 0; i < 200 && !pClient->ReaderExitShutdownEntered(); i++) {
				std::this_thread::sleep_for(5ms);
			}
			Assert::IsTrue(pClient->ReaderExitShutdownEntered(), L"Reader exit did not reach shutdown");
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Running,
				L"Reader exit left Running before the admitted dispatch was queued");
			{
				std::lock_guard lock{ hook.mutex };
				hook.release = true;
			}
			hook.cv.notify_all();
			const auto start = std::chrono::steady_clock::now();
			dispatcher.join();
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"Dispatch in the reader-exit gap did not finish");
			Assert::IsTrue(dispatchDone.load(), L"Accepted dispatch did not finish");
			pClient->SetAdmissionHookForTest(nullptr);
			gAdmissionHook_ = nullptr;
			pClient->WaitForShutdown();
			Assert::IsTrue(pClient->GetLifecyclePhase() == LifecyclePhase::Stopped);
			std::atomic<int> calls = 0;
			Assert::ExpectException<ServerDroppedError>([&] {
				pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 5, .pushCount = 0 },
					[&](EchoWithPush::Response&&, std::exception_ptr) { calls.fetch_add(1); });
			});
			std::this_thread::sleep_for(30ms);
			Assert::AreEqual(0, calls.load(), L"Rejected continuation was invoked");
		}
		TEST_METHOD(RunnerJoinsBeforeBorrowedDependencyDies)
		{
			auto borrowedAlive = std::make_shared<std::atomic<bool>>(true);
			std::vector<const char*> events;
			std::atomic<uint32_t> disposeCount = 0;
			const auto pipeName = MakeUniquePipeName_();
			{
				struct BorrowMark
				{
					std::shared_ptr<std::atomic<bool>> alive;
					~BorrowMark() { alive->store(false, std::memory_order_release); }
				} borrow{ borrowedAlive };
				{
					TestServer server{ pipeName, disposeCount, nullptr, nullptr, ServerTestHooks{
						.events = &events,
						.borrowedAlive = borrowedAlive,
					} };
					auto pClient = ConnectClient_(pipeName, disposeCount);
					(void)pClient;
				}
				Assert::IsTrue(borrowedAlive->load(), L"Borrowed dependency died before the runner joined");
				bool sawDestroyed = false;
				bool sawDeadTouch = false;
				for (auto event : events) {
					if (std::string_view{ event } == "ExecCtxDestroyed") {
						sawDestroyed = true;
					}
					if (std::string_view{ event } == "ExecCtxTouchedDeadBorrow") {
						sawDeadTouch = true;
					}
				}
				Assert::IsTrue(sawDestroyed, L"Execution context outlived the joined runner");
				Assert::IsFalse(sawDeadTouch, L"Runner still touched a borrowed dependency after it died");
			}
		}
		TEST_METHOD(ContinuationBeginShutdownThenExternalDestroy)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			std::atomic<bool> continued{ false };
			pClient->DispatchWithContinuation(EchoWithPush::Params{ .value = 6, .pushCount = 0 },
				[&](EchoWithPush::Response&&, std::exception_ptr) {
					pClient->BeginShutdown();
					continued.store(true);
				});
			for (int i = 0; i < 200 && !continued.load(); i++) {
				std::this_thread::sleep_for(10ms);
			}
			Assert::IsTrue(continued.load(), L"Continuation did not run");
			const auto start = std::chrono::steady_clock::now();
			pClient.reset();
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"External destroy after continuation shutdown did not join");
		}
		TEST_METHOD(FinalTeardownDisposesBeforeTraceStopAndPreservesReasons)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::vector<SessionCleanupMode> modes;
			std::atomic<uint32_t> updateTracking = 0;
			std::atomic<uint32_t> startEtw = 0;
			std::vector<const char*> events;
			std::atomic<uint32_t> eventCount = 0;
			auto borrowedAlive = std::make_shared<std::atomic<bool>>(true);
			bool tracesStoppedWhileBorrowed = false;
			{
				auto pServer = std::make_unique<TestServer<>>(pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
					.startEtw = &startEtw,
					.events = &events,
					.borrowedAlive = borrowedAlive,
				});
				auto pClient = ConnectClient_(pipeName, eventCount);
				JoinActionServerThenStopTraces(*pServer, [&] {
					pClient.reset();
					for (int i = 0; i < 200 && reasons.empty(); i++) {
						std::this_thread::sleep_for(10ms);
					}
				}, [&] {
					Assert::IsFalse(pServer->IsRunning());
					pServer.reset();
				}, [&] {
					tracesStoppedWhileBorrowed = borrowedAlive->load(std::memory_order_acquire);
				}, &events);
			}
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected,
				L"Final teardown overwrote a peer disconnect");
			Assert::AreEqual((size_t)1, modes.size());
			Assert::IsTrue(modes[0] == SessionCleanupMode::FinalTeardown);
			Assert::AreEqual(0u, updateTracking.load(), L"UpdateTracking ran during final teardown");
			Assert::AreEqual(0u, startEtw.load(), L"StartEtwSession ran during final teardown");
			Assert::IsTrue(tracesStoppedWhileBorrowed, L"PresentMon stand-in died before trace stop");
			AssertPipeAbsent_(pipeName);
			bool disposed = false;
			bool ctxDestroyed = false;
			for (auto event : events) {
				if (std::string_view{ event } == "Dispose") {
					Assert::IsFalse(ctxDestroyed, L"Session disposal ran after the execution context died");
					disposed = true;
				}
				if (std::string_view{ event } == "ExecCtxDestroyed") {
					ctxDestroyed = true;
				}
			}
			Assert::IsTrue(disposed);
			Assert::IsTrue(ctxDestroyed);
			size_t disposeAt = events.size();
			size_t destroyedAt = events.size();
			size_t stopAt = events.size();
			for (size_t i = 0; i < events.size(); i++) {
				const std::string_view name{ events[i] };
				if (name == "Dispose") {
					disposeAt = i;
				}
				if (name == "ExecCtxDestroyed") {
					destroyedAt = i;
				}
				if (name == "StopTraceSessions") {
					stopAt = i;
				}
			}
			Assert::IsTrue(stopAt < events.size(), L"Teardown did not record StopTraceSessions");
			Assert::IsTrue(disposeAt < stopAt, L"Disposal did not happen before StopTraceSessions");
			Assert::IsTrue(destroyedAt < stopAt, L"Action server was not joined before PresentMon teardown");
		}
		TEST_METHOD(NormalDisconnectStillRecomputes)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::mutex disposeMutex;
			std::vector<SessionEndReason> reasons;
			std::vector<SessionCleanupMode> modes;
			std::atomic<uint32_t> updateTracking = 0;
			std::atomic<uint32_t> eventCount = 0;
			{
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
				} };
				auto pClient = ConnectClient_(pipeName, eventCount);
				pClient.reset();
				for (int i = 0; i < 200 && reasons.empty(); i++) {
					std::this_thread::sleep_for(10ms);
				}
			}
			Assert::AreEqual((size_t)1, reasons.size());
			Assert::IsTrue(reasons[0] == SessionEndReason::PeerDisconnected);
			Assert::IsTrue(modes[0] == SessionCleanupMode::NormalOperation);
			Assert::AreEqual(1u, updateTracking.load(), L"A normal disconnect skipped global recomputation");
		}
		TEST_METHOD(ProtocolFaultAndBodyTimeoutKeepReasonDuringFinalTeardown)
		{
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::vector<SessionCleanupMode> modes;
				std::atomic<uint32_t> updateTracking = 0;
				std::atomic<uint32_t> eventCount = 0;
				ServerExecutionContext::ProtocolGate gate;
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
					.protocolGate = &gate,
				} };
				auto pClient = ConnectClient_(pipeName, eventCount);
				server.EnterFinalTeardown();
				std::atomic<bool> dispatchDone{ false };
				std::jthread dispatcher{ [&] {
					try {
						pClient->DispatchSync(ProtocolFault::Params{});
					}
					catch (...) {
					}
					dispatchDone.store(true);
				} };
				for (int i = 0; i < 200 && !gate.entered.load(); i++) {
					std::this_thread::sleep_for(5ms);
				}
				Assert::IsTrue(gate.entered.load(), L"Protocol fault action did not reach the gate");
				server.BeginShutdown();
				gate.release.store(true, std::memory_order_release);
				dispatcher.join();
				server.WaitForShutdown();
				Assert::IsTrue(dispatchDone.load());
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::ProtocolFailure,
					L"Final teardown replaced a protocol failure");
				Assert::IsTrue(modes[0] == SessionCleanupMode::FinalTeardown);
				Assert::AreEqual(0u, updateTracking.load());
			}
			{
				const auto pipeName = MakeUniquePipeName_();
				std::atomic<uint32_t> disposeCount = 0;
				std::mutex disposeMutex;
				std::vector<SessionEndReason> reasons;
				std::vector<SessionCleanupMode> modes;
				std::atomic<uint32_t> updateTracking = 0;
				TestServer server{ pipeName, disposeCount, &disposeMutex, &reasons, ServerTestHooks{
					.disposeModes = &modes,
					.updateTracking = &updateTracking,
					.bodyReadTimeoutMs = 50,
				} };
				server.EnterFinalTeardown();
				Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000));
				util::win::Handle raw;
				for (int i = 0; i < 200 && !raw; i++) {
					raw = util::win::Handle{ CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
						0, nullptr, OPEN_EXISTING, 0, nullptr) };
					if (!raw) {
						std::this_thread::sleep_for(5ms);
					}
				}
				Assert::IsTrue((bool)raw, L"Could not open a raw session for the body-read timeout");
				WaitForSessionCount_(server, 1);
				const uint32_t declared = 4096;
				DWORD written = 0;
				Assert::IsTrue(WriteFile(raw, &declared, sizeof(declared), &written, nullptr) != FALSE);
				for (int i = 0; i < 200 && reasons.empty(); i++) {
					std::this_thread::sleep_for(10ms);
				}
				Assert::AreEqual((size_t)1, reasons.size());
				Assert::IsTrue(reasons[0] == SessionEndReason::ResponseTimeout,
					L"Body-read timeout was relabeled");
				Assert::IsTrue(modes[0] == SessionCleanupMode::FinalTeardown);
				Assert::AreEqual(0u, updateTracking.load());
				server.BeginShutdown();
				server.WaitForShutdown();
				Assert::AreEqual((size_t)1, reasons.size(), L"Shutdown added a second disposal");
				Assert::IsTrue(reasons[0] == SessionEndReason::ResponseTimeout);
			}
		}
		TEST_METHOD(NoActionAdmittedAfterFinalTeardownStopping)
		{
			const auto pipeName = MakeUniquePipeName_();
			std::atomic<uint32_t> disposeCount = 0;
			std::atomic<uint32_t> eventCount = 0;
			TestServer server{ pipeName, disposeCount };
			auto pClient = ConnectClient_(pipeName, eventCount);
			server.EnterFinalTeardown();
			server.BeginShutdown();
			Assert::IsFalse(server.IsRunning());
			const auto start = std::chrono::steady_clock::now();
			server.DispatchDetached(PushNotice::Params{ .value = 1 });
			try {
				pClient->DispatchSync(EchoWithPush::Params{ .value = 1, .pushCount = 0 });
			}
			catch (...) {
			}
			Assert::IsTrue(std::chrono::steady_clock::now() - start < 500ms,
				L"A dispatch was admitted or blocked after Stopping");
			server.WaitForShutdown();
			Assert::AreEqual(0u, server.GetSessionCount());
		}
	private:
		template<class Server>
		static void WaitForSessionCount_(const Server& server, uint32_t expected)
		{
			for (int i = 0; i < 200 && server.GetSessionCount() != expected; i++) {
				std::this_thread::sleep_for(10ms);
			}
		}
		static void AssertTeardownIsPrompt_(std::unique_ptr<TestClient>& pClient, const wchar_t* situation)
		{
			const auto start = std::chrono::steady_clock::now();
			pClient.reset();
			const auto elapsed = std::chrono::steady_clock::now() - start;
			Assert::IsTrue(elapsed < 500ms, (L"Client teardown stalled "s + situation).c_str());
		}
		template<class Server>
		static void WaitForAcceptorCount_(const Server& server, uint32_t expected)
		{
			for (int i = 0; i < 200 && server.GetAcceptorCount() != expected; i++) {
				std::this_thread::sleep_for(10ms);
			}
		}
		// connect and vanish, the way an abusive (or merely impatient) client does
		static void RawConnectAndClose_(const std::string& name)
		{
			for (int i = 0; i < 200; i++) {
				const util::win::Handle handle{ CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE,
					0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr) };
				if (handle) {
					// the handle closes here, before the server has a chance to serve it
					return;
				}
				const auto error = GetLastError();
				if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND) {
					Assert::Fail(util::str::ToWide(std::format(
						"Raw connect to the transport test pipe failed with error {}", error)).c_str());
				}
				std::this_thread::sleep_for(5ms);
			}
			Assert::Fail(L"Raw connect to the transport test pipe never succeeded");
		}
		static void AssertPipeAbsent_(const std::string& name)
		{
			const util::win::Handle handle{ CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE,
				0, nullptr, OPEN_EXISTING, 0, nullptr) };
			const auto error = GetLastError();
			Assert::IsFalse((bool)handle, L"A suffixed half-pipe name is still being created");
			Assert::AreEqual((DWORD)ERROR_FILE_NOT_FOUND, error);
		}
	};
}
