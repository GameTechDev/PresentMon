// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "SymmetricActionConnector.h"
#include "AdmissionGate.h"
#include "Lifecycle.h"
#include "ServerSendPolicy.h"
#include "../../../CommonUtilities/str/String.h"
#include "../../../CommonUtilities/log/IdentificationTable.h"
#include "AsyncActionCollection.h"
#include "ActionContext.h"
#include <atomic>
#include <cassert>
#include <format>
#include <future>
#include <memory>
#include <thread>
#include <vector>


namespace pmon::ipc::act
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;

    template<class ExecCtx, class ServerPolicy = RequestResponseServerPolicy>
        requires TransportSessionContext<typename ExecCtx::SessionContextType>
    class SymmetricActionServer
    {
        using SessionContextType = typename ExecCtx::SessionContextType;
        using SessionsMap = std::unordered_map<uint32_t, SessionContextType>;
        using Connector = SymmetricActionConnector<ExecCtx>;

        struct State : std::enable_shared_from_this<State>
        {
            Lifecycle lifecycle;
            AdmissionGate admission;
            as::io_context ioctx;
            // Application session state and the transport connector for each session,
            // keyed by the same session id. Only AddSession_ and RemoveSession_ change
            // either map, so the two always hold the same keys.
            SessionsMap sessions;
            std::unordered_map<uint32_t, std::shared_ptr<Connector>> connectors;
            // Acceptors blocked in ConnectNamedPipe. They hold this State across the
            // suspend, so Drain_ must close them or the pipe name outlives shutdown.
            std::vector<std::shared_ptr<Connector>> listeners_;
            std::atomic<uint32_t> sessionCount{ 0 };
            std::atomic<uint32_t> acceptorCount{ 0 };
            // Session strands own a shared_ptr to this State. stop() abandons a
            // suspended strand, and that frame would keep State (and the pipe)
            // alive past the runner join. Drain_ waits until this hits zero.
            std::atomic<int> liveStrands_{ 0 };
            std::atomic<std::thread::id> runnerId{};
            ExecCtx ctx;
            bool allowConnectionlessSend = false;
            uint32_t reservedPipeInstanceCount = 0;
            uint32_t maxConcurrentSessions = 0;
            std::string basePipeName;
            std::string security;

            State(ExecCtx context, std::string pipeName, uint32_t reserved, std::string securityString,
                bool connectionless, uint32_t maxSessions)
                :
                ctx{ std::move(context) },
                allowConnectionlessSend{ connectionless },
                reservedPipeInstanceCount{ reserved },
                maxConcurrentSessions{ maxSessions },
                basePipeName{ std::move(pipeName) },
                security{ std::move(securityString) }
            {
                if constexpr (requires(ExecCtx& e, const SessionsMap* pSessions) { e.pSessionMap = pSessions; }) {
                    ctx.pSessionMap = &sessions;
                }
                assert(reservedPipeInstanceCount > 0);
                assert(maxConcurrentSessions >= reservedPipeInstanceCount);
            }
            bool OnIoThread() const
            {
                return std::this_thread::get_id() == runnerId.load(std::memory_order_acquire);
            }
            void BeginShutdown()
            {
                admission.Lock([&] {
                    if (!lifecycle.TryBeginShutdown()) {
                        return;
                    }
                    auto self = this->shared_from_this();
                    as::co_spawn(ioctx, [self]() mutable -> as::awaitable<void> {
                        co_await self->Drain_();
                        // The runner thread and the owning server still hold State.
                        // Drop this frame's ownership before stop so an abandoned
                        // shutdown frame cannot keep the pipe alive inside ~io_context.
                        auto& io = self->ioctx;
                        self.reset();
                        io.stop();
                    }, as::detached);
                });
            }
            void CloseListeners_()
            {
                auto listeners = std::move(listeners_);
                listeners_.clear();
                for (auto& listener : listeners) {
                    if (listener) {
                        listener->EndSession(SessionEndReason::LocalShutdown);
                    }
                }
            }
            void EndConnectedSessions_()
            {
                std::vector<std::shared_ptr<Connector>> conns;
                for (auto& entry : connectors) {
                    conns.push_back(entry.second);
                }
                for (auto& conn : conns) {
                    if (conn) {
                        conn->EndSession(SessionEndReason::LocalShutdown);
                    }
                }
            }
            as::awaitable<void> Drain_()
            {
                // Peer and protocol completions already queued classify themselves
                // before LocalShutdown is applied.
                co_await as::post(co_await as::this_coro::executor, as::use_awaitable);
                CloseListeners_();
                co_await as::post(co_await as::this_coro::executor, as::use_awaitable);
                EndConnectedSessions_();
                as::steady_timer timer{ co_await as::this_coro::executor };
                while (!sessions.empty() || admission.Inflight() > 0
                    || liveStrands_.load(std::memory_order_acquire) > 0) {
                    CloseListeners_();
                    EndConnectedSessions_();
                    timer.expires_after(kShutdownPollPeriod);
                    co_await timer.async_wait(as::use_awaitable);
                }
            }
            void Run()
            {
                runnerId.store(std::this_thread::get_id(), std::memory_order_release);
                try {
                    for (uint32_t i = 0; i < reservedPipeInstanceCount; i++) {
                        if (!SpawnListener_()) {
                            break;
                        }
                    }
                    ioctx.run();
                    pmlog_info("ActionServer exiting");
                }
                catch (...) {
                    lifecycle.MarkStopped();
                    pmlog_error(ReportException());
                    log::GetDefaultChannel()->Flush();
                    std::terminate();
                }
                lifecycle.MarkStopped();
            }
            pipe::PipeLimits PipeLimits_() const
            {
                pipe::PipeLimits limits{
                    .minBodyBytes = kMinPacketBodyBytes,
                    .maxBodyBytes = kMaxPacketBodyBytes,
                };
                if constexpr (requires { ctx.bodyReadTimeoutMs; }) {
                    if (ctx.bodyReadTimeoutMs) {
                        limits.bodyReadTimeoutMs = *ctx.bodyReadTimeoutMs;
                    }
                }
                return limits;
            }
            bool SpawnListener_()
            {
                if (!lifecycle.IsRunning()) {
                    return false;
                }
                std::shared_ptr<Connector> pConn;
                try {
                    pConn = Connector::MakeListener(basePipeName, ioctx, security, PipeLimits_());
                }
                catch (...) {
                    pmlog_error(util::ReportException("Failed creating action pipe instance"));
                    return false;
                }
                acceptorCount.fetch_add(1, std::memory_order_relaxed);
                listeners_.push_back(pConn);
                auto self = this->shared_from_this();
                liveStrands_.fetch_add(1, std::memory_order_acq_rel);
                try {
                    as::co_spawn(ioctx, [self, pConn]() -> as::awaitable<void> {
                        try {
                            co_await self->SessionStrand_(pConn);
                        }
                        catch (...) {
                            pmlog_error(util::ReportException("Action session strand failed"));
                        }
                        self->liveStrands_.fetch_sub(1, std::memory_order_acq_rel);
                    }, as::detached);
                }
                catch (...) {
                    liveStrands_.fetch_sub(1, std::memory_order_acq_rel);
                    throw;
                }
                return true;
            }
            void RemoveListener_(const std::shared_ptr<Connector>& pConn)
            {
                std::erase(listeners_, pConn);
            }
            as::awaitable<void> SessionStrand_(std::shared_ptr<Connector> pConn)
            {
                bool accepted = false;
                try {
                    co_await pConn->AcceptConnection();
                    accepted = true;
                }
                catch (const pipe::BenignPipeError&) {
                    pmlog_dbg(util::ReportException("Accept ended without a session"));
                }
                catch (...) {
                    pmlog_error(util::ReportException("Accept ended without a session"));
                }
                RemoveListener_(pConn);
                if (!lifecycle.IsRunning()) {
                    acceptorCount.fetch_sub(1, std::memory_order_relaxed);
                    co_return;
                }
                SpawnListener_();
                acceptorCount.fetch_sub(1, std::memory_order_relaxed);
                if (accepted) {
                    co_await ServeSession_(std::move(pConn));
                }
            }
            as::awaitable<void> ServeSession_(std::shared_ptr<Connector> pConn)
            {
                if (!lifecycle.IsRunning()) {
                    co_return;
                }
                if (ServerPolicy::kSinglePeer && !sessions.empty()) {
                    pmlog_warn("Refusing action pipe connection, single-peer server already has a session");
                    co_return;
                }
                if (sessions.size() >= maxConcurrentSessions) {
                    pmlog_warn("Refusing action pipe connection, session cap reached")
                        .pmwatch(maxConcurrentSessions);
                    co_return;
                }
                const auto sessionId = pConn->GetId();
                auto& stx = AddSession_(sessionId, pConn);
                pmlog_info(std::format("Action pipe connected id:{}", sessionId));
                const auto reason = co_await pConn->RunReaderLoop(ctx, stx);
                const auto clientPid = DisposeSession_(sessionId, reason);
                pmlog_info(std::format("Action pipe disconnected, session closed id:{} pid:{}",
                    sessionId, clientPid.value_or(0)));
            }
            std::optional<uint32_t> DisposeSession_(uint32_t sid, SessionEndReason reason)
            {
                pmlog_dbg(std::format("Disposing session id:{} reason:{}", sid, (int)reason));
                std::optional<uint32_t> remotePid;
                if (auto i = sessions.find(sid); i != sessions.end()) {
                    auto& session = i->second;
                    if (session.remotePid) {
                        remotePid = session.remotePid;
                    }
                    if constexpr (HasCustomSessionDispose<ExecCtx>) {
                        try {
                            SessionDisposition disposition{ .reason = reason };
                            if constexpr (requires(const ExecCtx& e) { e.GetSessionCleanupMode(); }) {
                                disposition.cleanupMode = ctx.GetSessionCleanupMode();
                            }
                            ctx.Dispose(session, disposition);
                        }
                        catch (...) {
                            pmlog_error(util::ReportException("Failure disposing session"));
                        }
                    }
                    RemoveSession_(sid);
                }
                else {
                    pmlog_warn("Session to be removed not found");
                }
                return remotePid;
            }
            SessionContextType& AddSession_(uint32_t sid, std::shared_ptr<Connector> pConn)
            {
                connectors.emplace(sid, std::move(pConn));
                auto& stx = sessions.emplace(sid, SessionContextType{}).first->second;
                assert(sessions.size() == connectors.size());
                sessionCount.store((uint32_t)sessions.size(), std::memory_order_relaxed);
                return stx;
            }
            void RemoveSession_(uint32_t sid)
            {
                sessions.erase(sid);
                connectors.erase(sid);
                assert(sessions.size() == connectors.size());
                sessionCount.store((uint32_t)sessions.size(), std::memory_order_relaxed);
            }
            // Null when the session is gone.
            std::shared_ptr<Connector> FindConnector_(uint32_t sid) const
            {
                if (auto i = connectors.find(sid); i != connectors.end()) {
                    return i->second;
                }
                return nullptr;
            }
            as::awaitable<SessionContextType*> LookupSinglePeer_()
            {
                if (sessions.empty()) {
                    co_return nullptr;
                }
                if (sessions.size() != 1) {
                    pmlog_error("Single-peer action server observed multiple sessions")
                        .pmwatch(sessions.size());
                    co_return nullptr;
                }
                co_return &sessions.begin()->second;
            }
            as::awaitable<SessionContextType*> LookupSession_(uint32_t sessionId)
            {
                if (auto i = sessions.find(sessionId); i != sessions.end()) {
                    co_return &i->second;
                }
                co_return nullptr;
            }
        };

    public:
        SymmetricActionServer(ExecCtx context, std::string basePipeName,
            uint32_t reservedPipeInstanceCount, std::string securityString, bool allowConnectionlessSend = false,
            uint32_t maxConcurrentSessions = 64)
        {
            state_ = std::make_shared<State>(std::move(context), std::move(basePipeName),
                reservedPipeInstanceCount, std::move(securityString), allowConnectionlessSend, maxConcurrentSessions);
            runner_ = std::thread([state = state_] {
                InstallSehTranslator();
                log::IdentificationTable::AddThisThread(
                    std::format("symact-{}-srv", MakeWorkerName_(state->basePipeName)));
                state->Run();
            });
        }
        SymmetricActionServer(const SymmetricActionServer&) = delete;
        SymmetricActionServer& operator=(const SymmetricActionServer&) = delete;
        SymmetricActionServer(SymmetricActionServer&&) = delete;
        SymmetricActionServer& operator=(SymmetricActionServer&&) = delete;
        // Joins the runner before borrowed execution-context dependencies die.
        // Destroying this object on its own action io thread is unsupported.
        ~SymmetricActionServer()
        {
            if (!state_) {
                return;
            }
            if (state_->OnIoThread()) {
                pmlog_error("Destroying the action server on its io thread is unsupported");
                assert(false && "Destroying the action server on its io thread");
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
            try {
                state_->BeginShutdown();
            }
            catch (...) {
                pmlog_error(ReportException("Failure starting action server shutdown"));
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
            if (!state_->lifecycle.WaitFor(kShutdownTimeout)) {
                pmlog_error("Action server runner did not stop after cancellation; refusing to detach");
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
            if (runner_.joinable()) {
                runner_.join();
            }
            state_.reset();
        }
        template<class Params>
        auto DispatchSync(Params&& params, std::optional<uint32_t> responseTimeoutMs = {})
            requires ServerPolicy::kUnaddressedSend
        {
            if (state_->OnIoThread()) {
                throw Except<util::Exception>("DispatchSync called from the action io thread");
            }
            using Result = ResponseFromParams<Params>;
            using ParamsT = std::decay_t<Params>;
            std::promise<Result> promise;
            auto future = promise.get_future();
            const auto accepted = state_->admission.TryAdmit(state_->lifecycle, [&] {
                auto state = state_;
                as::co_spawn(state_->ioctx, [state, owned = ParamsT{ std::forward<Params>(params) },
                    &promise, responseTimeoutMs]() mutable -> as::awaitable<void> {
                    try {
                        std::shared_ptr<Connector> conn;
                        if (state->sessions.size() == 1) {
                            conn = state->FindConnector_(state->sessions.begin()->first);
                        }
                        else if (state->sessions.size() > 1) {
                            pmlog_error("Single-peer action server observed multiple sessions")
                                .pmwatch(state->sessions.size());
                        }
                        if (!conn) {
                            if (state->allowConnectionlessSend) {
                                promise.set_value(Result{});
                            }
                            else {
                                pmlog_error("Server attempting to send when no client is connected");
                                promise.set_exception(std::make_exception_ptr(
                                    Except<util::Exception>("Server attempting to send when no client is connected")));
                            }
                            co_return;
                        }
                        const auto remotePid = state->sessions.begin()->second.remotePid;
                        conn->LogOutgoing<ParamsT>(remotePid);
                        auto result = co_await SyncRequest<ActionFromParams<ParamsT>>(owned, *conn, {}, responseTimeoutMs);
                        promise.set_value(std::move(result));
                    }
                    catch (...) {
                        try {
                            promise.set_exception(std::current_exception());
                        }
                        catch (...) {
                        }
                    }
                }, [state = state_](std::exception_ptr) {
                    state->admission.CompleteOne();
                });
            });
            if (!accepted) {
                throw Except<util::Exception>("Action server is not running");
            }
            return future.get();
        }
        template<class Params>
        auto DispatchSync(uint32_t sessionId, Params&& params, std::optional<uint32_t> responseTimeoutMs = {})
            requires ServerPolicy::kAddressedSend
        {
            if (state_->OnIoThread()) {
                throw Except<util::Exception>("DispatchSync called from the action io thread");
            }
            using Result = ResponseFromParams<Params>;
            using ParamsT = std::decay_t<Params>;
            std::promise<Result> promise;
            auto future = promise.get_future();
            const auto accepted = state_->admission.TryAdmit(state_->lifecycle, [&] {
                auto state = state_;
                as::co_spawn(state_->ioctx, [state, sessionId, owned = ParamsT{ std::forward<Params>(params) },
                    &promise, responseTimeoutMs]() mutable -> as::awaitable<void> {
                    try {
                        std::shared_ptr<Connector> conn;
                        uint32_t remotePid = 0;
                        if (auto i = state->sessions.find(sessionId); i != state->sessions.end()) {
                            remotePid = i->second.remotePid;
                            conn = state->FindConnector_(sessionId);
                        }
                        if (!conn) {
                            pmlog_error("Server send addressed to an unknown session").pmwatch(sessionId);
                            promise.set_exception(std::make_exception_ptr(
                                Except<util::Exception>("Server send addressed to an unknown session")));
                            co_return;
                        }
                        conn->LogOutgoing<ParamsT>(remotePid);
                        auto result = co_await SyncRequest<ActionFromParams<ParamsT>>(owned, *conn, {}, responseTimeoutMs);
                        promise.set_value(std::move(result));
                    }
                    catch (...) {
                        try {
                            promise.set_exception(std::current_exception());
                        }
                        catch (...) {
                        }
                    }
                }, [state = state_](std::exception_ptr) {
                    state->admission.CompleteOne();
                });
            });
            if (!accepted) {
                throw Except<util::Exception>("Action server is not running");
            }
            return future.get();
        }
        template<class Params>
        void DispatchDetached(Params&& params)
            requires ServerPolicy::kUnaddressedSend
        {
            using ParamsT = std::decay_t<Params>;
            const auto accepted = state_->admission.TryAdmit(state_->lifecycle, [&] {
                auto state = state_;
                as::co_spawn(state_->ioctx, [state, owned = ParamsT{ std::forward<Params>(params) }]() -> as::awaitable<void> {
                    std::shared_ptr<Connector> conn;
                    uint32_t remotePid = 0;
                    if (state->sessions.size() == 1) {
                        remotePid = state->sessions.begin()->second.remotePid;
                        conn = state->FindConnector_(state->sessions.begin()->first);
                    }
                    if (!conn) {
                        if (!state->allowConnectionlessSend && state->sessions.size() != 1) {
                            pmlog_warn("Server dropping detached send because no client is connected");
                        }
                        co_return;
                    }
                    conn->LogOutgoing<ParamsT>(remotePid);
                    if (conn->SessionHasEnded()) {
                        co_return;
                    }
                    try {
                        co_await AsyncEmit<ActionFromParams<ParamsT>>(owned, *conn);
                    }
                    catch (...) {
                        pmlog_error(ReportException());
                    }
                }, [state](std::exception_ptr) {
                    state->admission.CompleteOne();
                });
            });
            if (!accepted) {
                pmlog_warn("Dropping server send because the action server is not running");
            }
        }
        template<class Params>
        void DispatchDetached(uint32_t sessionId, Params&& params)
            requires ServerPolicy::kAddressedSend
        {
            if (state_->OnIoThread()) {
                throw Except<util::Exception>("DispatchDetached called from the action io thread");
            }
            using ParamsT = std::decay_t<Params>;
            std::promise<void> promise;
            auto future = promise.get_future();
            const auto accepted = state_->admission.TryAdmit(state_->lifecycle, [&] {
                auto state = state_;
                as::co_spawn(state_->ioctx, [state, sessionId, owned = ParamsT{ std::forward<Params>(params) },
                    &promise]() mutable -> as::awaitable<void> {
                    try {
                        std::shared_ptr<Connector> conn;
                        uint32_t remotePid = 0;
                        if (auto i = state->sessions.find(sessionId); i != state->sessions.end()) {
                            remotePid = i->second.remotePid;
                            conn = state->FindConnector_(sessionId);
                        }
                        if (!conn) {
                            pmlog_error("Server send addressed to an unknown session").pmwatch(sessionId);
                            promise.set_exception(std::make_exception_ptr(
                                Except<util::Exception>("Server send addressed to an unknown session")));
                            co_return;
                        }
                        conn->LogOutgoing<ParamsT>(remotePid);
                        if (!conn->SessionHasEnded()) {
                            co_await AsyncEmit<ActionFromParams<ParamsT>>(owned, *conn);
                        }
                        promise.set_value();
                    }
                    catch (...) {
                        try {
                            promise.set_exception(std::current_exception());
                        }
                        catch (...) {
                        }
                    }
                }, [state = state_](std::exception_ptr) {
                    state->admission.CompleteOne();
                });
            });
            if (!accepted) {
                throw Except<util::Exception>("Action server is not running");
            }
            future.get();
        }
        bool IsRunning() const
        {
            return state_ && state_->lifecycle.IsRunning();
        }
        LifecyclePhase GetLifecyclePhase() const
        {
            return state_->lifecycle.Get();
        }
        void EnterFinalTeardown()
        {
            if constexpr (requires(ExecCtx& e) { e.EnterFinalTeardown(); }) {
                state_->ctx.EnterFinalTeardown();
            }
        }
        void BeginShutdown()
        {
            state_->BeginShutdown();
        }
        // False when the runner has not reached Stopped. Does not detach.
        bool WaitForShutdown()
        {
            if (state_->OnIoThread()) {
                throw Except<util::Exception>("WaitForShutdown cannot run on the action io thread");
            }
            if (!state_->lifecycle.WaitFor(kShutdownTimeout)) {
                pmlog_warn("Timed out waiting for action server shutdown");
                return false;
            }
            return true;
        }
        uint32_t GetSessionCount() const
        {
            return state_->sessionCount.load(std::memory_order_relaxed);
        }
        uint32_t GetAcceptorCount() const
        {
            return state_->acceptorCount.load(std::memory_order_relaxed);
        }
        // Session ids are the admitting pipe ids. Copied on the io thread.
        std::vector<uint32_t> CopySessionIds()
        {
            if (state_->OnIoThread()) {
                throw Except<util::Exception>("CopySessionIds called from the action io thread");
            }
            std::promise<std::vector<uint32_t>> promise;
            auto future = promise.get_future();
            const auto accepted = state_->admission.TryAdmit(state_->lifecycle, [&] {
                auto state = state_;
                as::co_spawn(state_->ioctx, [state, &promise]() -> as::awaitable<void> {
                    try {
                        std::vector<uint32_t> ids;
                        ids.reserve(state->sessions.size());
                        for (auto& entry : state->sessions) {
                            ids.push_back(entry.first);
                        }
                        promise.set_value(std::move(ids));
                    }
                    catch (...) {
                        try {
                            promise.set_exception(std::current_exception());
                        }
                        catch (...) {
                        }
                    }
                    co_return;
                }, [state = state_](std::exception_ptr) {
                    state->admission.CompleteOne();
                });
            });
            if (!accepted) {
                throw Except<util::Exception>("Action server is not running");
            }
            return future.get();
        }

    private:
        static std::string MakeWorkerName_(const std::string& pipeNameBase)
        {
            constexpr std::string_view prefix = R"(\\.\pipe\)";
            if (pipeNameBase.starts_with(prefix)) {
                return pipeNameBase.substr(prefix.size());
            }
            return pipeNameBase;
        }
        std::shared_ptr<State> state_;
        std::thread runner_;
    };
}
