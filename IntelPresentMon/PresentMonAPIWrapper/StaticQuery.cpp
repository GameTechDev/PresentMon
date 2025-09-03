#pragma once
#include "StaticQuery.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>

namespace pmapi
{
    StaticQueryResult PollStatic(const Session& session, const ProcessTracker& process,
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
        if (const auto err = pmPollStaticQuery(session.GetHandle(), &element, process.GetPid(), result.blob_.data());
            err != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ err, "Error polling static query" };
        }
        return result;
    }
}
