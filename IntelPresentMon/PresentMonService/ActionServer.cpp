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

    ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName)
    {
        // if we have a pipe name override, that indicates we don't need special permissions
        auto sec = pipe::DuplexPipe::GetSecurityString(pipeName ?
            pipe::SecurityMode::Child : pipe::SecurityMode::Service);
        // construct (and start) the server
        pImpl_ = std::make_shared<act::SymmetricActionServer<acts::ActionExecutionContext>>(
            acts::ActionExecutionContext{ .pSvc = pSvc, .pPmon = pPmon },
            pipeName.value_or(gid::defaultControlPipeName),
            2, std::move(sec)
        );
    }
}