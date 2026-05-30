#pragma once
#include "StaticQuery.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>

namespace pmapi
{
    StaticQueryResult StaticQueryResult::PollStatic_(const Session& session, uint32_t pid,
        PM_METRIC metric, uint32_t deviceId, uint32_t arrayIndex)
    {
        const auto pIntro = session.GetIntrospectionRoot();
        const auto dti = pIntro->FindMetric(metric).GetDataTypeInfo();
        StaticQueryResult result{ dti.GetFrameType(), dti.GetEnumId() };

        const PM_QUERY_ELEMENT element{
            .metric = metric,
            .stat = PM_STAT_NONE,
            .deviceId = deviceId,
            .arrayIndex = arrayIndex,
        };
        if (const auto err = pmPollStaticQuery(session.GetHandle(), &element, pid, result.blob_.data());
            err != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ err, "Error polling static query" };
        }
        return result;
    }

    StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
        PM_METRIC metric, uint32_t deviceId, uint32_t arrayIndex)
    {
        return StaticQueryResult::PollStatic_(session, process.GetPid(), metric, deviceId, arrayIndex);
    }

    StaticQueryResult PollStatic(const Session& session,
        PM_METRIC metric, uint32_t deviceId, uint32_t arrayIndex)
    {
        return StaticQueryResult::PollStatic_(session, 0, metric, deviceId, arrayIndex);
    }
}
