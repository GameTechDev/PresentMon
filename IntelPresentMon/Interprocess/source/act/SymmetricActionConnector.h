// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "Transfer.h"
#include "ResponseRouter.h"
#include "AsyncActionCollection.h"
#include "SessionEndReason.h"
#include "CommandTokenAllocator.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace pmon::ipc::act
{
	// Bound for WaitForShutdown and for the destructor's join. A timeout is not
	// permission to detach: the destructor fails fast instead of returning while
	// the runner can still touch borrowed execution-context state.
	inline constexpr std::chrono::milliseconds kShutdownTimeout{ 2000 };
	inline constexpr std::chrono::milliseconds kShutdownPollPeriod{ 5 };
	// Late responses after a response timeout consume one expiration record.
	// Evicted tokens are forgotten, so a later response is an unknown token.
	inline constexpr size_t kMaxExpiredResponseTokens = 64;

	template<class ExecCtx>
	class SymmetricActionConnector : public ResponseRouter
	{
	public:
		using SessionContextType = typename ExecCtx::SessionContextType;

		// Sole reader of the pipe. Returns the reason the session ended.
		// Every exit has already failed pending requesters.
		as::awaitable<SessionEndReason> RunReaderLoop(ExecCtx& ctx, SessionContextType& stx)
		{
			std::exception_ptr error;
			try {
				while (!sessionEnded_.load(std::memory_order_acquire)) {
					const auto header = co_await pPipe_->ReadPacketConsumeHeader<PacketHeader>();
					if (sessionEnded_.load(std::memory_order_acquire)) {
						break;
					}
					if (!IsPlausibleHeader(header)) {
						pmlog_error("Rejecting implausible packet header")
							.pmwatch(header.headerVersion).pmwatch((int)header.packetType);
						throw util::Except<ProtocolViolation>("Implausible packet header");
					}
					if (header.packetType == PacketType::ActionResponse) {
						RouteResponse_(header);
					}
					else {
						co_await ExecuteIncoming_(ctx, stx, header);
					}
				}
			}
			catch (...) {
				error = std::current_exception();
				NoteReaderFailure_(error);
			}
			if (pPipe_) {
				pPipe_->Close();
			}
			if (endReason_) {
				co_return *endReason_;
			}
			const auto reason = error ? ClassifyEnd_(error) : SessionEndReason::PeerDisconnected;
			// a peer that broke the protocol or stalled is an error; a disconnect or local shutdown is routine
			if (error) {
				if (reason == SessionEndReason::ProtocolFailure || reason == SessionEndReason::PeerStalled) {
					pmlog_error(util::ReportException({}, error));
				}
				else {
					pmlog_dbg(util::ReportException({}, error));
				}
			}
			co_return reason;
		}
		template<class Params>
		void LogOutgoing(uint32_t remotePid) const
		{
			using Action = ActionFromParams<Params>;
			pmlog_dbg("Action Dispatch").pmwatch(Action::Identifier).pmwatch(remotePid);
		}
		uint32_t GetId() const
		{
			return pPipe_->GetId();
		}
		bool SessionHasEnded() const
		{
			return sessionEnded_.load(std::memory_order_acquire);
		}
		void RegisterPending(uint32_t commandToken, PendingResponse& pending) override
		{
			if (sessionEnded_) {
				throw util::Except<util::Exception>("Cannot send request, session has ended");
			}
			if (!pendingResponses_.emplace(commandToken, &pending).second) {
				pmlog_error("Duplicate pending command token").pmwatch(commandToken);
				throw util::Except<ProtocolViolation>("Duplicate pending command token");
			}
		}
		void ExpirePending(uint32_t commandToken) override
		{
			pendingResponses_.erase(commandToken);
			if (!expiredResponseTokens_.insert(commandToken).second) {
				return;
			}
			expiredResponseTokenOrder_.push_back(commandToken);
			while (expiredResponseTokenOrder_.size() > kMaxExpiredResponseTokens) {
				expiredResponseTokens_.erase(expiredResponseTokenOrder_.front());
				expiredResponseTokenOrder_.pop_front();
			}
		}
		uint32_t AllocateCommandToken() override
		{
			return tokens_.Allocate([this](uint32_t token) {
				return pendingResponses_.contains(token) || expiredResponseTokens_.contains(token);
			});
		}
		util::pipe::DuplexPipe& GetPipe() override
		{
			return *pPipe_;
		}
		as::io_context& GetIoContext() override
		{
			return ioctx_;
		}
		static std::shared_ptr<SymmetricActionConnector> MakeListener(
			const std::string& pipeName, as::io_context& ioctx, const std::string& security,
			pipe::PipeLimits limits)
		{
			return std::make_shared<SymmetricActionConnector>(pipeName, ioctx, security, std::move(limits));
		}
		as::awaitable<void> AcceptConnection()
		{
			co_await pPipe_->Accept();
			clientPid_ = pPipe_->GetClientProcessId();
		}
		// process id of the pipe client, as reported by the system; set once accepted
		uint32_t GetClientProcessId() const
		{
			return clientPid_;
		}
		// Client side: process id of the pipe server, as reported by the system.
		uint32_t ResolveConnectedServerProcessId()
		{
			uint32_t serverPid = 0;
			if (!pPipe_->TryGetConnectedServerProcessId(serverPid)) {
				throw util::Except<pipe::PipeError>("Failed to resolve server process id from control pipe");
			}
			return serverPid;
		}
		// Fails waiters and cancels the pipe. The reader resumes and returns `reason`.
		// Must be called on the io thread. A second call is a no-op.
		void EndSession(SessionEndReason reason)
		{
			if (sessionEnded_.load(std::memory_order_acquire)) {
				return;
			}
			// A reader-classified peer, protocol, or timeout reason wins over LocalShutdown.
			if (!endReason_) {
				endReason_ = reason;
			}
			FailAllPending_(std::make_exception_ptr(
				util::Except<pipe::PipeOperationCanceled>("Session ended by local shutdown")));
			if (pPipe_) {
				pPipe_->Close();
			}
		}
		static std::shared_ptr<SymmetricActionConnector> ConnectToServer(
			const std::string& pipeName, as::io_context& ioctx,
			CommandTokenAllocator tokens = {})
		{
			return std::make_shared<SymmetricActionConnector>(pipeName, ioctx, std::move(tokens));
		}
		SymmetricActionConnector(const std::string& pipeName, as::io_context& ioctx,
			const std::string& security, pipe::PipeLimits limits)
			:
			ioctx_{ ioctx },
			pPipe_{ pipe::DuplexPipe::MakeAsPtr(pipeName, ioctx, security, std::move(limits)) }
		{}
		SymmetricActionConnector(const std::string& pipeName, as::io_context& ioctx,
			CommandTokenAllocator tokens)
			:
			ioctx_{ ioctx },
			pPipe_{ pipe::DuplexPipe::ConnectAsPtr(pipeName, ioctx, MakePipeLimits_()) },
			tokens_{ std::move(tokens) }
		{}
	private:
		static pipe::PipeLimits MakePipeLimits_()
		{
			return { .minBodyBytes = kMinPacketBodyBytes, .maxBodyBytes = kMaxPacketBodyBytes };
		}
		static SessionEndReason ClassifyEnd_(std::exception_ptr error)
		{
			try {
				std::rethrow_exception(error);
			}
			catch (const pipe::PipeReadTimeout&) {
				return SessionEndReason::PeerStalled;
			}
			catch (const ProtocolViolation&) {
				return SessionEndReason::ProtocolFailure;
			}
			catch (const pipe::PipeOperationCanceled&) {
				return SessionEndReason::LocalShutdown;
			}
			catch (const pipe::BenignPipeError&) {
				return SessionEndReason::PeerDisconnected;
			}
			catch (const pipe::PipeError&) {
				return SessionEndReason::ProtocolFailure;
			}
			catch (...) {
				return SessionEndReason::ProtocolFailure;
			}
		}
		as::awaitable<void> ExecuteIncoming_(ExecCtx& ctx, SessionContextType& stx, const PacketHeader& header)
		{
			try {
				// any action other than OpenSession without having remotePid is an anomaly
				// TODO: make this processing a customization point in ExecutionContext and move it out of here
				if (header.identifier != "OpenSession") {
					assert(bool(stx.remotePid));
					if (!stx.remotePid) {
						pmlog_warn("Received action without a valid session opened").diag();
					}
				}
				// lookup the command by identifier and execute it with remaining buffer contents
				// response is then transmitted over the pipe to remote
				// TODO: make this return result code (increment error count based on this)
				const auto pidBefore = stx.remotePid;
				co_await AsyncActionCollection<ExecCtx>::Get().Find(header.identifier).Execute(ctx, stx, header, *pPipe_);
				// the peer claimed a pid other than the one the system reports for the pipe client;
				// remotePid is only used for diagnostics, so this is reported rather than refused
				if (clientPid_ && stx.remotePid != pidBefore && stx.remotePid != clientPid_) {
					pmlog_warn("Session claimed a pid that does not match the pipe client")
						.pmwatch(stx.remotePid).pmwatch(clientPid_);
				}
				co_return;
			}
			// we assume any pipe-transport related errors and protocol violations are not
			// recoverable and proceed to terminate connection
			catch (const pipe::PipeError&) {
				throw;
			}
			catch (const ProtocolViolation&) {
				throw;
			}
			catch (...) {
				pmlog_error(util::ReportException());
			}
			pPipe_->DiscardPacketPayload();
			// if the output buffer is dirty, we're not sure what state we're in so just clear it
			if (pPipe_->GetWriteBufferPending()) {
				pPipe_->ClearWriteBuffer();
			}
			if (header.packetType == PacketType::ActionEvent) {
				co_return;
			}
			auto resHeader = MakeResponseHeader(header, TransportStatus::TransportFailure, PM_STATUS_SUCCESS);
			co_await pPipe_->WritePacket(std::move(resHeader), EmptyPayload{}, ctx.responseWriteTimeoutMs);
		}
		bool ConsumeExpired_(uint32_t commandToken)
		{
			if (!expiredResponseTokens_.erase(commandToken)) {
				return false;
			}
			for (auto it = expiredResponseTokenOrder_.begin(); it != expiredResponseTokenOrder_.end(); ++it) {
				if (*it == commandToken) {
					expiredResponseTokenOrder_.erase(it);
					break;
				}
			}
			return true;
		}
		void RouteResponse_(const PacketHeader& header)
		{
			const auto token = header.commandToken;
			if (auto i = pendingResponses_.find(token); i != pendingResponses_.end()) {
				auto& pending = *i->second;
				pendingResponses_.erase(i);
				pending.Complete(header, *pPipe_);
				return;
			}
			pPipe_->DiscardPacketPayload();
			if (ConsumeExpired_(token)) {
				pmlog_warn("Consumed late response for an expired request").pmwatch(token);
				return;
			}
			pmlog_error("Response for unknown command token").pmwatch(token);
			throw util::Except<ProtocolViolation>("Response for unknown command token");
		}
		void NoteReaderFailure_(std::exception_ptr error)
		{
			const auto classified = ClassifyEnd_(error);
			if (!endReason_ || (*endReason_ == SessionEndReason::LocalShutdown
				&& classified != SessionEndReason::LocalShutdown)) {
				endReason_ = classified;
			}
			if (!sessionEnded_.load(std::memory_order_acquire)) {
				FailAllPending_(error);
			}
		}
		void FailAllPending_(std::exception_ptr error)
		{
			sessionEnded_.store(true, std::memory_order_release);
			auto pending = std::move(pendingResponses_);
			pendingResponses_.clear();
			expiredResponseTokens_.clear();
			expiredResponseTokenOrder_.clear();
			for (auto& entry : pending) {
				entry.second->Fail(error);
			}
		}
		as::io_context& ioctx_;
		std::unique_ptr<pipe::DuplexPipe> pPipe_;
		CommandTokenAllocator tokens_;
		// zero on the connecting side, which has no peer to verify
		uint32_t clientPid_ = 0;
		std::unordered_map<uint32_t, PendingResponse*> pendingResponses_;
		std::unordered_set<uint32_t> expiredResponseTokens_;
		std::deque<uint32_t> expiredResponseTokenOrder_;
		std::optional<SessionEndReason> endReason_;
		std::atomic<bool> sessionEnded_{ false };
	};
}
