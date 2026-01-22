#pragma once
#include <cstdint>
#include <vector>
#include "DynamicStat.h"

namespace pmon::mid
{
    // container for multiple stats connected to a single metric
    // implements shared stat calculation facilities (i.e. sorted sample buffer for %)
    template<typename T>
    class DynamicMetric
    {
    public:
        DynamicMetric();
        ~DynamicMetric();
    private:
        std::vector<double> samples_;
    };
}
