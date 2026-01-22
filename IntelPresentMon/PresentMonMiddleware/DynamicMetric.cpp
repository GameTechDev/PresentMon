#include "DynamicMetric.h"

namespace pmon::mid
{
    DynamicMetric::DynamicMetric() = default;
    DynamicMetric::~DynamicMetric() = default;

    uint32_t DynamicMetric::GetId() const
    {
        return id_;
    }
}
