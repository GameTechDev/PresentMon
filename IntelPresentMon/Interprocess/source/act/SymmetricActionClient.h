// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "SymmetricActionConnector.h"
#include "AdmissionGate.h"
#include "Lifecycle.h"
#include "ActionRunner.h"
#include "../../../CommonUtilities/log/IdentificationTable.h"
#include "ActionContext.h"
#include <atomic>
#include <format>
#include <future>
#include <memory>
#include <thread>


namespace pmon::ipc::act
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;

    PM_DEFINE_EX(ActionClientError);
    PM_DEFINE_EX_FROM(ActionClientError, ServerDroppedError);

    template<class ExecCtx>
        requires TransportSessionContext<typename ExecCtx::SessionContextType>
    class SymmetricActionClient
    {
        using SessionContextType = typename ExecCtx::SessionContextType;
        using Connector = SymmetricActionConnector<ExecCtx>;
        // Shared with the runner thread so shutdown never captures a raw this.
        // The runner std::thread itself stays on this object so the io thread
        // does not join itself.
        struct State : std::enable_shared_from_this<State>
        {
            Lifecycle lifecycle;
            AdmissionGate admission;
            as::io_context ioctx;
            // Declared after ioctx so it is destroyed before the context its pipe uses.
            std::shared_ptr<Connector> conn;
            SessionContextType stx;
            ExecCtx ctx;
            std::string basePipeName;
            std::atomic<std::thread::id> runnerId{};
            // The reader strand owns a shared_ptr to this State. stop() must wait
            // until it has exited, or the abandoned frame keeps the pipe alive.
            std::atomic<int> liveStrands_{ 0 };
            // Set on the io thread before reader-exit shutdown takes the admission
            // lock, so a test can observe the check-to-post gap.
            std::atomic<bool> readerExitShutdownEntered_{ false };

            State(std::string pipeName, ExecCtx context, CommandTokenAllocator tokens)
                :
                ctx{ std::move(context) },
                basePipeName{ std::move(pipeName) }
            {
                conn = Connector::ConnectToServer(basePipeName, ioctx, std::move(tokens));
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
                        co_await self->StopSequence_();
                        auto& io = self->ioctx;
                        self.reset();
                        io.stop();
                    }, as::detached);
                });
            }
            as::awaitable<void> StopSequence_()
            {
                // A peer error already queued on this executor classifies itself
                // before LocalShutdown can relabel it.
                co_await as::post(co_await as::this_coro::executor, as::use_awaitable);
                if (conn) {
                    conn->EndSession(SessionEndReason::LocalShutdown);
                }
                co_await as::post(co_await as::this_coro::executor, as::use_awaitable);
                as::steady_timer timer{ co_await as::this_coro::executor };
                while (admission.Inflight() > 0
                    || liveStrands_.load(std::memory_order_acquire) > 0) {
                    timer.expires_after(kShutdownPollPeriod);
                    co_await timer.async_wait(as::use_awaitable);
                }
            }
            void Run()
            {
                runnerId.store(std::this_thread::get_id(), std::memory_order_release);
                try {
                    auto self = this->shared_from_this();
                    liveStrands_.fetch_add(1, std::memory_order_acq_rel);
                    as::co_spawn(ioctx, [self]() -> as::awaitable<void> {
                        try {
                            co_await self->SessionStrand_();
                        }
                        catch (...) {
                            pmlog_error(ReportException());
                        }
                        self->liveStrands_.fetch_sub(1, std::memory_order_acq_rel);
                    }, as::detached);
                    ioctx.run();
                    pmlog_info("ActionClient exiting");
                }
                catch (...) {
                    lifecycle.MarkStopped();
                    pmlog_error(ReportException());
                    // if the action server crashes for any reason, the service should restart at this point
                    // TODO: don't make this mandatory for all uses of the server (client in this case)
                    log::GetDefaultChannel()->Flush();
                    std::terminate();
                }
                lifecycle.MarkStopped();
            }
            as::awaitable<void> SessionStrand_()
            {
                try {
                    co_await conn->RunReaderLoop(ctx, stx);
                }
                catch (...) {
                    pmlog_error(util::ReportException());
                }
                // Same lock and queued stop sequence as BeginShutdown. run() cannot
                // return until that sequence is posted, so a dispatch in TryAdmit
                // is either queued first or rejected.
                readerExitShutdownEntered_.store(true, std::memory_order_release);
                BeginShutdown();
                pmlog_info("Exiting action client session strand");
            }
        };
    public:
        SymmetricActionClient(std::string pipeName, ExecCtx context = {}, CommandTokenAllocator tokens = {})
        {
            state_ = std::make_shared<State>(std::move(pipeName), std::move(context), std::move(tokens));
            runner_ = std::thread([state = state_] {
                InstallSehTranslator();
                log::IdentificationTable::AddThisThread(
                    std::format("symact-{}-cli", MakeWorkerName(state->basePipeName)));
                state->Run();
            });
        }

        SymmetricActionClient(const SymmetricActionClient&) = delete;
        SymmetricActionClient& operator=(const SymmetricActionClient&) = delete;
        SymmetricActionClient(SymmetricActionClient&&) = delete;
        SymmetricActionClient& operator=(SymmetricActionClient&&) = delete;
        // Joins the runner before borrowed execution-context state is destroyed.
        // Destroying this object on its own action io thread is unsupported.
        ~SymmetricActionClient()
        {
            StopAndJoinRunner(state_, runner_, "action client");
        }

        // Rejected when the lifecycle is no longer Running. Acceptance is decided
        // under the same lock that queues the coroutine. An accepted call either
        // returns a response or throws. A rejected call does not start.
        template<class Params>
        auto DispatchSync(Params&& params, std::optional<uint32_t> responseTimeoutMs = {})
        {
            // it would block the only thread that can read the response
            if (state_->OnIoThread()) {
                throw Except<ActionClientError>("DispatchSync cannot run on the action io thread");
            }
            using Result = ResponseFromParams<Params>;
            using ParamsT = std::decay_t<Params>;
            std::promise<Result> promise;
            auto future = promise.get_future();
            // CAUTION: the coroutine below captures promise by reference. That is only safe because
            // this function blocks on the future until the coroutine has set it; keep that wait if refactoring.
            const auto remotePid = state_->stx.remotePid;
            const auto accepted = SpawnAdmitted(state_, [conn = state_->conn, remotePid,
                owned = ParamsT{ std::forward<Params>(params) }, &promise, responseTimeoutMs]() mutable -> as::awaitable<void> {
                try {
                    conn->LogOutgoing<ParamsT>(remotePid);
                    auto result = co_await SyncRequest<ActionFromParams<ParamsT>>(owned, *conn, responseTimeoutMs);
                    promise.set_value(std::move(result));
                }
                catch (...) {
                    try {
                        promise.set_exception(std::current_exception());
                    }
                    catch (...) {
                    }
                }
            });
            if (!accepted) {
                throw Except<ServerDroppedError>("Action client is not running; cannot dispatch");
            }
            return future.get();
        }
        template<class Params>
        void DispatchDetached(Params&& params)
        {
            using ParamsT = std::decay_t<Params>;
            const auto remotePid = state_->stx.remotePid;
            const auto accepted = SpawnAdmitted(state_, [conn = state_->conn, remotePid,
                owned = ParamsT{ std::forward<Params>(params) }]() -> as::awaitable<void> {
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
            });
            if (!accepted) {
                throw Except<ServerDroppedError>("Action client is not running; cannot dispatch");
            }
        }
        // `cont` runs on the action io thread, exactly once for an accepted call,
        // with either a response or an exception. It may BeginShutdown or start
        // another async dispatch while Running. It must not WaitForShutdown,
        // DispatchSync, or destroy this client. A rejected call does not invoke `cont`.
        template<class Params>
        void DispatchWithContinuation(Params&& params, std::function<void(ResponseFromParams<Params>&&, std::exception_ptr)> cont)
        {
            using ParamsT = std::decay_t<Params>;
            const auto remotePid = state_->stx.remotePid;
            const auto accepted = SpawnAdmitted(state_, [conn = state_->conn, remotePid,
                owned = ParamsT{ std::forward<Params>(params) }, cont = std::move(cont)]() mutable -> as::awaitable<void> {
                conn->LogOutgoing<ParamsT>(remotePid);
                try {
                    if (conn->SessionHasEnded()) {
                        throw util::Except<util::Exception>("Cannot send request, session has ended");
                    }
                    auto res = co_await SyncRequest<ActionFromParams<ParamsT>>(owned, *conn);
                    try {
                        cont(std::move(res), {});
                    }
                    catch (...) {
                        pmlog_error(ReportException("Final failure in calling continuation"));
                    }
                }
                catch (...) {
                    pmlog_dbg(ReportException("Error in IPC dispatch"));
                    try {
                        cont({}, std::current_exception());
                    }
                    catch (...) {
                        pmlog_error(ReportException("Final failure in calling continuation"));
                    }
                }
            });
            if (!accepted) {
                throw Except<ServerDroppedError>("Action client is not running; cannot dispatch");
            }
        }
        bool IsRunning() const
        {
            return state_ && state_->lifecycle.IsRunning();
        }
        LifecyclePhase GetLifecyclePhase() const
        {
            return state_->lifecycle.Get();
        }
        bool ReaderExitShutdownEntered() const
        {
            return state_->readerExitShutdownEntered_.load(std::memory_order_acquire);
        }
        // Nonblocking. Safe on an application thread, the action io thread, and a continuation.
        // Repeated calls observe the same Running -> Stopping transition.
        void BeginShutdown()
        {
            state_->BeginShutdown();
        }
        // Blocks the caller until Stopped or kShutdownTimeout. Rejected on the action io thread.
        // A timeout leaves the runner alive; it does not detach it. False means not stopped.
        bool WaitForShutdown()
        {
            if (state_->OnIoThread()) {
                throw Except<ActionClientError>("WaitForShutdown cannot run on the action io thread");
            }
            if (!state_->lifecycle.WaitFor(kShutdownTimeout)) {
                pmlog_warn("Timed out waiting for action client shutdown");
                return false;
            }
            return true;
        }
        void SetAdmissionHookForTest(void (*hook)())
        {
            state_->admission.SetHookForTest(hook);
        }

    protected:
        void EstablishSession_(uint32_t serverPid)
        {
            state_->stx.remotePid = serverPid;
        }
        Connector& SessionConnector_()
        {
            return *state_->conn;
        }

    private:
        std::shared_ptr<State> state_;
        std::thread runner_;
    };
}
