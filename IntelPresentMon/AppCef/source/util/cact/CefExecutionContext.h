#pragma once
#include <Interprocess/source/act/ActionHelper.h>
#include <Interprocess/source/act/SymmetricActionConnector.h>
#include <CommonUtilities/pipe/ManualAsyncEvent.h>
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>

namespace p2c::client::util
{
    class SignalManager;
}

namespace p2c::client::util::cact
{
    using namespace ::pmon;
    struct CefExecutionContext;

    struct CefSessionContext
    {
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<CefExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
    };

    struct CefExecutionContext
    {
        // types
        using SessionContextType = CefSessionContext;

        // data
        SignalManager* pSignalManager = nullptr;
        std::optional<uint32_t> responseWriteTimeoutMs;
    };
}