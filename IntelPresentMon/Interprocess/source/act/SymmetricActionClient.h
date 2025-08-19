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
    class SymmetricActionClient
    {
        using SessionContextType = typename ExecCtx::SessionContextType;
    public:
        SymmetricActionClient(std::string pipeName, ExecCtx context = {})
            :
            basePipeName_{ std::move(pipeName) },
            ctx_{ std::move(context) }
        {
            stx_.pConn = SymmetricActionConnector<ExecCtx>::ConnectToServer(basePipeName_, ioctx_);
            as::co_spawn(ioctx_, SessionStrand_(), as::detached);
            runner_ = mt::Thread{ std::format("symact-{}-cli", MakeWorkerName_(basePipeName_)),
                &SymmetricActionClient::Run_, this };
        }

        SymmetricActionClient(const SymmetricActionClient&) = delete;
        SymmetricActionClient& operator=(const SymmetricActionClient&) = delete;
        SymmetricActionClient(SymmetricActionClient&&) = delete;
        SymmetricActionClient& operator=(SymmetricActionClient&&) = delete;
        ~SymmetricActionClient()
        {
            ioctx_.stop();
        }

        template<class Params>
        auto DispatchSync(Params&& params)
        {
            assert(IsRunning());
            return stx_.pConn->DispatchSync(std::forward<Params>(params), ioctx_, stx_);
        }
        template<class Params>
        auto DispatchDetached(Params&& params)
        {
            assert(IsRunning());
            return stx_.pConn->DispatchDetached(std::forward<Params>(params), ioctx_, stx_);
        }
        template<class Params>
        void DispatchWithContinuation(Params&& params, std::function<void(ResponseFromParams<Params>&&, std::exception_ptr)> cont)
        {
            assert(IsRunning());
            return stx_.pConn->DispatchWithContinuation(std::forward<Params>(params), ioctx_, stx_, std::move(cont));
        }
        bool IsRunning() const
        {
            return !ioctx_.stopped();
        }

    protected:
        void EstablishSession_(uint32_t serverPid)
        {
            stx_.remotePid = serverPid;
        }

    private:
        // function
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
            // TODO: investigate heap issue with this line
            //log::IdentificationTable::AddThisThread("act-cli-worker");
            try {
                ioctx_.run();
                pmlog_info("ActionClient exiting");
            }
            catch (...) {
                pmlog_error(ReportException());
                // if the action server crashes for any reason, the service should restart at this point
                // TODO: don't make this mandatory for all uses of the server (client in this case)
                log::GetDefaultChannel()->Flush();
                std::terminate();
            }
        }
        as::awaitable<void> SessionStrand_()
        {
            // TODO: work on propagation of server disconnection event errors
            try {
                bool alive = true;
                while (true) {
                    co_await stx_.pConn->SyncHandleRequest(ctx_, stx_);
                }
            }
            catch (const pipe::BenignPipeError&) {
                pmlog_dbg(util::ReportException());
            }
            catch (...) {
                pmlog_error(util::ReportException());
            }
            pmlog_info("Exiting action server session strand");
        }
        // data
        std::string basePipeName_;
        pipe::as::io_context ioctx_;
        SessionContextType stx_;
        ExecCtx ctx_;
        mt::Thread runner_;
    };
}