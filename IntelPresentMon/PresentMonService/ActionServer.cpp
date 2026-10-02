// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/str/String.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "ActionServer.h"
#include "GlobalIdentifiers.h"
#include "ActionExecutionContext.h"


namespace pmon::svc
{
    using namespace pmon;
    using namespace util;
    using namespace ipc;

    using ServerImpl = act::SymmetricActionServer<acts::ActionExecutionContext>;

    ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
    {
        // if we have a pipe name override, that indicates we don't need special permissions
        auto sec = pipe::DuplexPipe::GetSecurityString(pipeName ?
            pipe::SecurityMode::Child : pipe::SecurityMode::Service);
        // construct (and start) the server
        pImpl_ = std::make_shared<ServerImpl>(
            acts::ActionExecutionContext{ .pSvc = pSvc, .pPmon = pPmon },
            pipeName.value_or(gid::defaultControlPipeName),
            2, std::move(sec)
        );
    }
    void ActionServer::EnterFinalTeardown()
    {
        static_cast<ServerImpl*>(pImpl_.get())->EnterFinalTeardown();
    }
    void ActionServer::BeginShutdown()
    {
        static_cast<ServerImpl*>(pImpl_.get())->BeginShutdown();
    }
    bool ActionServer::WaitForShutdown()
    {
        return static_cast<ServerImpl*>(pImpl_.get())->WaitForShutdown();
    }
    uint32_t ActionServer::GetSessionCount() const
    {
        return static_cast<const ServerImpl*>(pImpl_.get())->GetSessionCount();
    }
    uint32_t ActionServer::GetAcceptorCount() const
    {
        return static_cast<const ServerImpl*>(pImpl_.get())->GetAcceptorCount();
    }
}