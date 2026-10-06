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

    namespace
    {
        // pImpl_ is type-erased so the header does not need the server template
        ServerImpl& Impl(const std::shared_ptr<void>& pImpl)
        {
            return *static_cast<ServerImpl*>(pImpl.get());
        }
    }

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
        Impl(pImpl_).EnterFinalTeardown();
    }
    void ActionServer::BeginShutdown()
    {
        Impl(pImpl_).BeginShutdown();
    }
    bool ActionServer::WaitForShutdown()
    {
        return Impl(pImpl_).WaitForShutdown();
    }
    uint32_t ActionServer::GetSessionCount() const
    {
        return Impl(pImpl_).GetSessionCount();
    }
    uint32_t ActionServer::GetAcceptorCount() const
    {
        return Impl(pImpl_).GetAcceptorCount();
    }
}