#pragma once
#include <algorithm>
#include <cstddef>
#include <numeric>
#include <type_traits>
#include <vector>

namespace pmon::util
{
    template<typename Sample>
    class SampleStatistics
    {
        static_assert(std::is_arithmetic_v<Sample>, "SampleStatistics requires arithmetic sample types.");
    public:
        void AddSample(Sample sample)
        {
            samples_.push_back(sample);
            prepared_ = false;
        }

        void Reset() noexcept
        {
            samples_.clear();
            prepared_ = false;
        }

        void Prepare()
        {
            EnsurePrepared_();
        }

        size_t GetSampleCount() const noexcept
        {
            return samples_.size();
        }

        double GetMean() const
        {
            if (samples_.empty()) {
                return 0.0;
            }
            const auto sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
            return sum / static_cast<double>(samples_.size());
        }

        Sample GetPercentile(double percentile) const
        {
            if (samples_.empty()) {
                return Sample{};
            }
            EnsurePrepared_();
            percentile = std::clamp(percentile, 0.0, 1.0);
            if (samples_.size() == 1 || percentile == 0.0) {
                return samples_.front();
            }
            if (percentile == 1.0) {
                return samples_.back();
            }

            const auto scaledIndex = percentile * static_cast<double>(samples_.size() - 1);
            const auto lower = static_cast<size_t>(scaledIndex);
            const auto upper = std::min(lower + 1, samples_.size() - 1);
            const auto weight = scaledIndex - static_cast<double>(lower);
            const auto low = static_cast<double>(samples_[lower]);
            const auto high = static_cast<double>(samples_[upper]);

            return static_cast<Sample>(low + (high - low) * weight);
        }

    private:
        void EnsurePrepared_() const
        {
            if (!prepared_) {
                std::sort(samples_.begin(), samples_.end());
                prepared_ = true;
            }
        }

        mutable std::vector<Sample> samples_;
        mutable bool prepared_ = false;
    };
}
