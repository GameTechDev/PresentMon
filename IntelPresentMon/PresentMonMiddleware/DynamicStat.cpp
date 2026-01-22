#include "DynamicStat.h"

namespace ipc = pmon::ipc;
namespace util = pmon::util;

namespace pmon::mid
{
    namespace
    {
        bool IsAvgStat_(PM_STAT stat)
        {
            return stat == PM_STAT_AVG || stat == PM_STAT_NON_ZERO_AVG;
        }

        bool IsSupportedOutputType_(PM_DATA_TYPE outType, bool allowBool)
        {
            switch (outType) {
            case PM_DATA_TYPE_DOUBLE:
            case PM_DATA_TYPE_INT32:
            case PM_DATA_TYPE_ENUM:
                return true;
            case PM_DATA_TYPE_BOOL:
                return allowBool;
            default:
                return false;
            }
        }

        void ValidateOutputType_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType)
        {
            if (IsAvgStat_(stat)) {
                if (outType != PM_DATA_TYPE_DOUBLE) {
                    throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                        "Dynamic stat average expects double output value");
                }
                return;
            }

            const bool allowBool = inType == PM_DATA_TYPE_BOOL;
            if (!IsSupportedOutputType_(outType, allowBool)) {
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Unsupported dynamic stat output data type");
            }
        }

        template<typename T>
        std::unique_ptr<DynamicStat<T>> MakeDynamicStatTyped_(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
        {
            ValidateOutputType_(stat, inType, outType);

            switch (stat) {
            case PM_STAT_AVG:
                return std::make_unique<detail::DynamicStatAverage_<T>>(inType, outType, offsetBytes, false);
            case PM_STAT_NON_ZERO_AVG:
                return std::make_unique<detail::DynamicStatAverage_<T>>(inType, outType, offsetBytes, true);
            case PM_STAT_PERCENTILE_99:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.99);
            case PM_STAT_PERCENTILE_95:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.95);
            case PM_STAT_PERCENTILE_90:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.90);
            case PM_STAT_PERCENTILE_01:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.01);
            case PM_STAT_PERCENTILE_05:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.05);
            case PM_STAT_PERCENTILE_10:
                return std::make_unique<detail::DynamicStatPercentile_<T>>(inType, outType, offsetBytes, 0.10);
            case PM_STAT_MAX:
                return std::make_unique<detail::DynamicStatMinMax_<T>>(inType, outType, offsetBytes, true);
            case PM_STAT_MIN:
                return std::make_unique<detail::DynamicStatMinMax_<T>>(inType, outType, offsetBytes, false);
            case PM_STAT_MID_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, PM_STAT_MID_POINT);
            case PM_STAT_NEWEST_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, PM_STAT_NEWEST_POINT);
            case PM_STAT_OLDEST_POINT:
                return std::make_unique<detail::DynamicStatPoint_<T>>(inType, outType, offsetBytes, PM_STAT_OLDEST_POINT);
            case PM_STAT_NONE:
            case PM_STAT_MID_LERP:
            case PM_STAT_COUNT:
            default:
                throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                    "Unsupported dynamic stat");
            }
        }
    }

    std::unique_ptr<DynamicStatBase> MakeDynamicStat(PM_STAT stat, PM_DATA_TYPE inType, PM_DATA_TYPE outType, size_t offsetBytes)
    {
        switch (inType) {
        case PM_DATA_TYPE_DOUBLE:
            return MakeDynamicStatTyped_<double>(stat, inType, outType, offsetBytes);
        case PM_DATA_TYPE_INT32:
        case PM_DATA_TYPE_ENUM:
            return MakeDynamicStatTyped_<int32_t>(stat, inType, outType, offsetBytes);
        case PM_DATA_TYPE_BOOL:
            return MakeDynamicStatTyped_<bool>(stat, inType, outType, offsetBytes);
        default:
            throw util::Except<ipc::PmStatusError>(PM_STATUS_QUERY_MALFORMED,
                "Unsupported dynamic stat input data type");
        }
    }
}
