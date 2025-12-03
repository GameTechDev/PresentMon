#pragma once
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include "../Interprocess/source/ShmNamer.h"
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include "PresentMon.h"
#include "Service.h"


namespace pmon::svc::acts
{
    struct ActionExecutionContext;

    struct ActionSessionContext
    {
        // common session context items
        std::unique_ptr<ipc::act::SymmetricActionConnector<ActionExecutionContext>> pConn;
        uint32_t remotePid = 0;
        uint32_t nextCommandToken = 0;
        // custom items
        std::set<uint32_t> trackedPids;
        std::set<uint32_t> etwLogSessionIds;
        std::optional<uint32_t> requestedAdapterId;
        std::optional<uint32_t> requestedTelemetryPeriodMs;
        std::optional<uint32_t> requestedEtwFlushPeriodMs;
        std::string clientBuildId;
    };

    struct ActionExecutionContext
    {
        // types
        using SessionContextType = ActionSessionContext;

        // data
        Service* pSvc = nullptr;
        PresentMon* pPmon = nullptr;
        const std::unordered_map<uint32_t, SessionContextType>* pSessionMap = nullptr;
        std::string shmPrefix;
        std::string shmSalt;
        std::optional<uint32_t> responseWriteTimeoutMs;

        // functions
        void Dispose(SessionContextType& stx);
        void UpdateTelemetryPeriod() const;
        void UpdateEtwFlushPeriod() const;
    };
}