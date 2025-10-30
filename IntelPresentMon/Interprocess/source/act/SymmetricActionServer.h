// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "SymmetricActionConnector.h"
#include "../../../CommonUtilities/str/String.h"
#include "../../../CommonUtilities/pipe/ManualAsyncEvent.h"
#include "../../../CommonUtilities/mt/Thread.h"
#include "AsyncActionCollection.h"
#include "ActionContext.h"
#include <thread>
#include <boost/asio/experimental/awaitable_operators.hpp>


namespace pmon::ipc::act
{
    using namespace std::literals;
    using namespace pmon;
    using namespace util;
    using namespace ipc;
    namespace as = boost::asio;
    using namespace as::experimental::awaitable_operators;

    template<class ExecCtx>
    class SymmetricActionServer
    {
        using SessionContextType = typename ExecCtx::SessionContextType;
        using SessionsMap = std::unordered_map<uint32_t, SessionContextType>;

    public:
        SymmetricActionServer(ExecCtx context, std::string basePipeName,
            uint32_t reservedPipeInstanceCount, std::string securityString, bool allowConnectionlessSend = false)
            :
            reservedPipeInstanceCount_{ reservedPipeInstanceCount },
            basePipeName_{ std::move(basePipeName) },
            security_{ std::move(securityString) },
            ctx_{ std::move(context) },
            worker_{ std::format("symact-{}-srv", MakeWorkerName_(basePipeName_)), &SymmetricActionServer::Run_, this},
            allowConnectionlessSend_{ allowConnectionlessSend }
        {
            // Only set pSessionMap if ExecCtx can be assigned a const SessionsMap*
            if constexpr (requires(ExecCtx& e, const SessionsMap* pSessions) { e.pSessionMap = pSessions; }) {
                ctx_.pSessionMap = &sessions_;
            }
            assert(reservedPipeInstanceCount_ > 0);
        }
        SymmetricActionServer(const SymmetricActionServer&) = delete;
        SymmetricActionServer& operator=(const SymmetricActionServer&) = delete;
        SymmetricActionServer(SymmetricActionServer&&) = delete;
        SymmetricActionServer& operator=(SymmetricActionServer&&) = delete;
        ~SymmetricActionServer()
        {
            ioctx_.stop();
        }
        template<class Params>
        auto DispatchSync(Params&& params)
        {
            assert(IsRunning());
            // TODO: server needs an actual way to specify which client endpoint to transmit to
            if (sessions_.empty()) {
                if (allowConnectionlessSend_) {
                    return ResponseFromParams<Params>{};
                }
                assert(false && "Server attempting to send when no client is connected");
                pmlog_error("Server attempting to send when no client is connected");
            }
            auto& stx = sessions_.begin()->second;
            return stx.pConn->DispatchSync(std::forward<Params>(params), ioctx_, stx);
        }
        template<class Params>
        void DispatchDetached(Params&& params)
        {
            assert(IsRunning());
            // TODO: server needs an actual way to specify which client endpoint to transmit to
            if (sessions_.empty()) {
                if (allowConnectionlessSend_) {
                    return;
                }
                assert(false && "Server attempting to send when no client is connected");
                pmlog_error("Server attempting to send when no client is connected");
            }
            auto& stx = sessions_.begin()->second;
            stx.pConn->DispatchDetached(std::forward<Params>(params), ioctx_, stx);
        }
        bool IsRunning() const
        {
            return !ioctx_.stopped();
        }

    private:
        // functions
        static std::string MakeWorkerName_(const std::string& pipeNameBase)
        {
            constexpr std::string_view prefix = R"(\\.\pipe\)";
            std::string base;
            if (pipeNameBase.starts_with(prefix)) {
                base = pipeNameBase.substr(prefix.size());
            }
            else {
                base = pipeNameBase;
            }
            return base;
        }
        void Run_()
        {
            try {
                // maintain N available connector instances at all times
                for (uint32_t i = 0; i < reservedPipeInstanceCount_; i++) {
                    as::co_spawn(ioctx_, SessionStrand_(), as::detached);
                }
                // run the io context event handler until signalled to exit
                ioctx_.run();
                pmlog_info("ActionServer exiting");
            }
            catch (...) {
                pmlog_error(ReportException());
                // if the action server crashes for any reason, the service should restart at this point
                // TODO: don't make this mandatory for all uses of the server
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
        }
        as::awaitable<void> SessionStrand_()
        {
            std::optional<uint32_t> sessionId;
            try {
                // create connector and suspend until client connects
                auto pConn = co_await SymmetricActionConnector<ExecCtx>::AcceptClientConnection(basePipeName_, ioctx_, security_);
                // insert a session context object for this connection, will be initialized properly upon OpenSession action
                sessionId = pConn->GetId();
                auto&&[i, b] = sessions_.emplace(*sessionId, SessionContextType{ .pConn = std::move(pConn) });
                pmlog_info(std::format("Action pipe connected id:{}", *sessionId));
                auto& stx = i->second;
                // fork this acceptor coroutine
                as::co_spawn(ioctx_, SessionStrand_(), as::detached);
                // run the action handler until client session is terminated
                while (true) {
                    co_await stx.pConn->SyncHandleRequest(ctx_, stx);
                }
            }
            catch (const pipe::BenignPipeError&) {
                pmlog_dbg(util::ReportException());
            }
            catch (...) {
                pmlog_error(util::ReportException());
            }
            if (sessionId) {
                const auto clientPid = DisposeSession_(*sessionId);
                pmlog_info(std::format("Action pipe disconnected, session closed id:{} pid:{}", *sessionId, clientPid.value_or(0)));
            }
            else {
                pmlog_info("Sessionless pipe disconnected");
            }
        }
        std::optional<uint32_t> DisposeSession_(uint32_t sid)
        {
            pmlog_dbg(std::format("Disposing session id:{}", sid));
            std::optional<uint32_t> remotePid;
            if (auto i = sessions_.find(sid); i != sessions_.end()) {
                auto& session = i->second;
                if (session.remotePid) {
                    remotePid = session.remotePid;
                    if constexpr (HasCustomSessionDispose<ExecCtx>) {
                        ctx_.Dispose(session);
                    }
                }
                sessions_.erase(i);
            }
            else {
                pmlog_warn("Session to be removed not found");
            }
            return remotePid;
        }
        // data
        bool allowConnectionlessSend_ = false;
        uint32_t reservedPipeInstanceCount_;
        std::string basePipeName_;
        std::string security_;
        as::io_context ioctx_;
        // maps session uid => session (uid is same as session recv (in) pipe id)
        SessionsMap sessions_;
        ExecCtx ctx_;
        mt::Thread worker_;
    };
}