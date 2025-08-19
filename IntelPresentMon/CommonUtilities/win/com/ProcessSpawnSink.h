// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../WinAPI.h"
#include "WbemSink.h"
#include <mutex>
#include <deque>
#include <optional>
#include <functional>
#include "../Process.h"

namespace pmon::util::win::com
{
    class ProcessSpawnSink : public win::com::WbemSink
    {
    public:
        // types
        class EventQueue
        {
            friend ProcessSpawnSink;
        public:
            EventQueue(std::function<void()> notification = {});
            std::optional<Process> Pop();
        private:
            // functions
            void Push(Process proc);
            void Notify() const;
            // data
            std::mutex mtx_;
            std::deque<Process> processSpawnEvents_;
            std::function<void()> notificationFunction_;
        };
        // functions
        ProcessSpawnSink(EventQueue& queue, float delayToleranceSeconds = 0.4);
        HRESULT STDMETHODCALLTYPE Indicate(LONG count,
            IWbemClassObject __RPC_FAR* __RPC_FAR* pObjArr) override;
        std::string GetQueryString() const override;
    private:
        float delayToleranceSeconds_;
        EventQueue& eventQueue_;
    };
}