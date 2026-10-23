// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
// Shared scaffolding for the action transport tests (TransportDemuxTests and its sibling
// Transport*Tests files): toy execution contexts and actions, test servers and clients, and
// helpers. The transport is exercised over private pipes, so the session, demultiplexer and
// disposal contract can be tested without the service or a CEF process.
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "CppUnitTest.h"
#include "../CommonUtilities/win/Handle.h"
#include "../CommonUtilities/str/String.h"
#include "../Interprocess/source/act/AsyncActionCollection.h"
#include "../Interprocess/source/act/SymmetricActionClient.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "../Interprocess/source/act/ActionServerShutdown.h"
#include "RawPipeClient.h"
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/windows/default_launcher.hpp>
#include <filesystem>
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
		uint32_t remotePid = 0;
	};

	struct ClientExecutionContext
	{
		using SessionContextType = ClientSessionContext;

		std::optional<uint32_t> responseWriteTimeoutMs;
		std::atomic<uint32_t>* pEventCount = nullptr;
	};

	// The transport owns its connectors; a session context only has to carry remotePid.
	static_assert(TransportSessionContext<ServerSessionContext>);
	static_assert(TransportSessionContext<ClientSessionContext>);
	struct SessionContextWithoutRemotePid_ {};
	static_assert(!TransportSessionContext<SessionContextWithoutRemotePid_>);
	struct SessionContextWithWideRemotePid_ { uint64_t remotePid = 0; };
	static_assert(!TransportSessionContext<SessionContextWithWideRemotePid_>);

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

	// carries a request larger than the pipe's buffer, so sending it needs the peer to read
	class BulkRequest : public AsyncActionBase_<BulkRequest, ServerExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "Bulk";
		struct Params
		{
			std::string bytes;
			template<class A> void serialize(A& ar) { ar(bytes); }
		};
		struct Response
		{
			uint32_t size;
			template<class A> void serialize(A& ar) { ar(size); }
		};
	private:
		friend class AsyncActionBase_<BulkRequest, ServerExecutionContext>;
		static Response Execute_(const ServerExecutionContext&, SessionContext&, Params&& in)
		{
			return Response{ .size = (uint32_t)in.bytes.size() };
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

	// a push larger than the pipe's buffer, so sending it holds the server's write lock until
	// the client reads it
	class BulkNotice : public AsyncEventActionBase_<BulkNotice, ClientExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "BulkNotice";
		struct Params
		{
			std::string bytes;
			template<class A> void serialize(A& ar) { ar(bytes); }
		};
	private:
		friend class AsyncEventActionBase_<BulkNotice, ClientExecutionContext>;
		static void Execute_(const ClientExecutionContext& ctx, SessionContext&, Params&&)
		{
			ctx.pEventCount->fetch_add(1);
		}
	};

	// Server-side traits holder; the client runs StallReaderClient, which waits on a timer, so
	// its reader reads nothing for the duration while its io thread stays free to send.
	class StallReaderNotice : public AsyncEventActionBase_<StallReaderNotice, ClientExecutionContext>
	{
	public:
		static constexpr const char* Identifier = "StallReader";
		struct Params
		{
			uint32_t durationMs;
			template<class A> void serialize(A& ar) { ar(durationMs); }
		};
	private:
		friend class AsyncEventActionBase_<StallReaderNotice, ClientExecutionContext>;
		static void Execute_(const ClientExecutionContext&, SessionContext&, Params&&)
		{
			assert(false && "StallReaderNotice is server-side only");
		}
	};

	class StallReaderClient : public AsyncAction<ClientExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ClientExecutionContext&, SessionContext&,
			const PacketHeader&, pipe::DuplexPipe& pipe) const override
		{
			auto params = pipe.ConsumePacketPayload<StallReaderNotice::Params>();
			pipe::as::steady_timer timer{ co_await pipe::as::this_coro::executor };
			timer.expires_after(std::chrono::milliseconds(params.durationMs));
			co_await timer.async_wait(pipe::as::use_awaitable);
		}
		const char* GetIdentifier() const override
		{
			return StallReaderNotice::Identifier;
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
	template<> struct ActionParamsTraits<TransportDemuxTests::BulkRequest::Params>
	{
		using Action = TransportDemuxTests::BulkRequest;
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
	template<> struct ActionParamsTraits<TransportDemuxTests::BulkNotice::Params>
	{
		using Action = TransportDemuxTests::BulkNotice;
	};
	template<> struct ActionParamsTraits<TransportDemuxTests::StallReaderNotice::Params>
	{
		using Action = TransportDemuxTests::StallReaderNotice;
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
	// Adds the toy actions to the action collections, once per process. Defined in
	// TransportTestKit.cpp so their transport code is generated in one place.
	void RegisterActions_();

	struct ServerTestHooks
	{
		std::vector<SessionCleanupMode>* disposeModes = nullptr;
		std::atomic<uint32_t>* updateTracking = nullptr;
		std::atomic<uint32_t>* startEtw = nullptr;
		std::vector<const char*>* events = nullptr;
		std::optional<uint32_t> bodyReadTimeoutMs;
		std::optional<uint32_t> responseWriteTimeoutMs;
		ServerExecutionContext::ProtocolGate* protocolGate = nullptr;
		std::shared_ptr<std::atomic<bool>> borrowedAlive;
		SessionLimits limits;
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
					.responseWriteTimeoutMs = hooks.responseWriteTimeoutMs,
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
				pipeName, reservedAcceptors, std::string{}, false, hooks.limits }
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
		TestClient(const std::string& pipeName, std::atomic<uint32_t>& eventCount,
			uint32_t claimedPid = GetCurrentProcessId())
			:
			SymmetricActionClient{ pipeName, ClientExecutionContext{ .pEventCount = &eventCount } }
		{
			const auto res = DispatchSync(OpenSession::Params{ .clientPid = claimedPid });
			EstablishSession_(res.serverPid);
		}
	};

	// A server with no listeners: each session is a pipe pair it creates in OpenPairedSession.
	class PairedTestServer
	{
	public:
		PairedTestServer(std::atomic<uint32_t>& disposeCount, std::mutex* disposeMutex = nullptr,
			std::vector<SessionEndReason>* disposeReasons = nullptr)
			:
			server_{ ServerExecutionContext{
					.pPush = &push_,
					.pDisposeCount = &disposeCount,
					.pDisposeMutex = disposeMutex,
					.pDisposeReasons = disposeReasons,
				},
				std::format("pm-demux-pair-test-{}", GetCurrentProcessId()), std::string{} }
		{
			push_ = [this](uint32_t value) {
				server_.DispatchDetached(PushNotice::Params{ .value = value });
			};
		}
		util::win::Handle OpenPairedSession() { return server_.OpenPairedSession(); }
		uint32_t GetSessionCount() const { return server_.GetSessionCount(); }
		void BeginShutdown() { server_.BeginShutdown(); }
		bool WaitForShutdown() { return server_.WaitForShutdown(); }
		bool IsRunning() const { return server_.IsRunning(); }
	private:
		std::function<void(uint32_t)> push_;
		SymmetricActionServer<ServerExecutionContext, PairedSinglePeerServerPolicy> server_;
	};

	class PairedTestClient : public SymmetricActionClient<ClientExecutionContext>
	{
	public:
		PairedTestClient(util::win::Handle connectedPipe, std::atomic<uint32_t>& eventCount)
			:
			SymmetricActionClient{ std::move(connectedPipe), "pair-test",
				ClientExecutionContext{ .pEventCount = &eventCount } }
		{
			const auto res = DispatchSync(OpenSession::Params{ .clientPid = GetCurrentProcessId() });
			EstablishSession_(res.serverPid);
		}
	};

	inline std::string MakeUniquePipeName_()
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

	inline std::unique_ptr<TestClient> ConnectClient_(const std::string& pipeName, std::atomic<uint32_t>& eventCount)
	{
		Assert::IsTrue(util::pipe::DuplexPipe::WaitForAvailability(pipeName, 2000),
			L"Timed out waiting for the test transport pipe");
		return std::make_unique<TestClient>(pipeName, eventCount);
	}

	// A refused connection is accepted at the pipe level and closed by the server without
	// ever becoming a session. The connect itself must succeed, so a busy or missing pipe
	// fails the test instead of passing for a refusal. Nothing is sent, so within the wait
	// only refusal can close it; callers keep the handshake deadline well beyond it.
	template<class Server>
	bool ConnectionIsRefused_(const Server& server, const std::string& pipeName)
	{
		const auto sessionsBefore = server.GetSessionCount();
		RawPipeClient raw{ pipeName };
		Assert::IsTrue(raw.IsConnected(), L"Could not connect to the test transport pipe");
		return raw.WaitForServerClose(1s) && server.GetSessionCount() == sessionsBefore;
	}

	// Counts C++ exceptions raised anywhere in this process while it is alive, including
	// ones that are caught. 0xE06D7363 is the SEH code MSVC uses for every C++ throw.
	class CxxThrowCounter_
	{
	public:
		CxxThrowCounter_()
		{
			count_.store(0);
			handle_ = AddVectoredExceptionHandler(1, &Handler_);
		}
		CxxThrowCounter_(const CxxThrowCounter_&) = delete;
		CxxThrowCounter_& operator=(const CxxThrowCounter_&) = delete;
		~CxxThrowCounter_()
		{
			RemoveVectoredExceptionHandler(handle_);
		}
		uint32_t Count() const
		{
			return count_.load();
		}
	private:
		static LONG CALLBACK Handler_(EXCEPTION_POINTERS* pInfo)
		{
			if (pInfo->ExceptionRecord->ExceptionCode == 0xE06D7363) {
				count_.fetch_add(1);
			}
			return EXCEPTION_CONTINUE_SEARCH;
		}
		static inline std::atomic<uint32_t> count_{ 0 };
		PVOID handle_ = nullptr;
	};

	template<class Server>
	inline void WaitForSessionCount_(const Server& server, uint32_t expected)
	{
		for (int i = 0; i < 200 && server.GetSessionCount() != expected; i++) {
			std::this_thread::sleep_for(10ms);
		}
	}

	inline void AssertTeardownIsPrompt_(std::unique_ptr<TestClient>& pClient, const wchar_t* situation)
	{
		const auto start = std::chrono::steady_clock::now();
		pClient.reset();
		const auto elapsed = std::chrono::steady_clock::now() - start;
		Assert::IsTrue(elapsed < 500ms, (L"Client teardown stalled "s + situation).c_str());
	}

	template<class Server>
	inline void WaitForAcceptorCount_(const Server& server, uint32_t expected)
	{
		for (int i = 0; i < 200 && server.GetAcceptorCount() != expected; i++) {
			std::this_thread::sleep_for(10ms);
		}
	}

	// connect and vanish, the way an abusive (or merely impatient) client does
	inline void RawConnectAndClose_(const std::string& name)
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

	inline void AssertPipeAbsent_(const std::string& name)
	{
		const util::win::Handle handle{ CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE,
			0, nullptr, OPEN_EXISTING, 0, nullptr) };
		const auto error = GetLastError();
		Assert::IsFalse((bool)handle, L"A suffixed half-pipe name is still being created");
		Assert::AreEqual((DWORD)ERROR_FILE_NOT_FOUND, error);
	}
}
