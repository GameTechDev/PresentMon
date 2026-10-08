// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "ActionServer.h"
#include "GlobalIdentifiers.h"
#include "ActionExecutionContext.h"


namespace pmon::svc
{
    using namespace pmon;
    using namespace util;
    using namespace ipc;

    ActionServer::ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName,
        bool controlPipeAllowAuClients)
    {
        constexpr uint32_t reservedPipeInstanceCount = 2;

        std::string sec;
        if (pipeName) {
            sec = pipe::DuplexPipe::GetPrivateControlPipeSecurityString(controlPipeAllowAuClients);
        }
        else {
            sec = pipe::DuplexPipe::GetServiceControlPipeSecurityString();
        }
        // construct (and start) the server
        pImpl_ = std::make_shared<act::SymmetricActionServer<acts::ActionExecutionContext>>(
            acts::ActionExecutionContext{ .pSvc = pSvc, .pPmon = pPmon },
            pipeName.value_or(gid::defaultControlPipeName),
            reservedPipeInstanceCount, std::move(sec), false
        );
    }
}
