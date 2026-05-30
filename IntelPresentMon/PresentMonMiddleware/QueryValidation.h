#pragma once
#include <span>
#include <IntelPresentMon/PresentMonAPI2/PresentMonAPI.h>

namespace pmapi::intro
{
    class Root;
}

namespace pmon::ipc
{
    class MiddlewareComms;
}

namespace pmon::mid
{
    void ValidateQueryElements(std::span<PM_QUERY_ELEMENT> queryElements, PM_METRIC_TYPE queryType,
        const pmapi::intro::Root& introRoot, const ipc::MiddlewareComms& comms);
}
