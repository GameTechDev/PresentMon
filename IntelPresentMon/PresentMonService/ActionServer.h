// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <optional>
#include <memory>
#include "PresentMon.h"
#include "Service.h"
#include "FrameBroadcaster.h"

namespace pmon::svc
{
    class ActionServer
    {
    public:
        ActionServer(Service* pSvc, PresentMon* pPmon, std::optional<std::string> pipeName,
            bool controlPipeAllowAuClients = false);
        ~ActionServer() = default;
        // Marks disposal as final teardown. Does not stop the runner.
        void EnterFinalTeardown();
        void BeginShutdown();
        // False if the runner has not stopped. Does not detach and does not destroy PresentMon.
        bool WaitForShutdown();
        ActionServer(const ActionServer&) = delete;
        ActionServer& operator=(const ActionServer&) = delete;
        ActionServer(ActionServer&&) = delete;
        ActionServer& operator=(ActionServer&&) = delete;
        // transport health, exposed so tests can assert that abuse does not erode admission
        uint32_t GetSessionCount() const;
        uint32_t GetAcceptorCount() const;
    private:
        std::shared_ptr<void> pImpl_;
    };
}
