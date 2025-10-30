#pragma once
#include <ranges>
#include <optional>
#include <utility>
#include <type_traits>

namespace pmon::util::rng
{
    namespace {
        template<class R>
        using OptionalRangeValueType_ =
            typename std::remove_cvref_t<std::ranges::range_reference_t<R>>::value_type;
    }

    // OptionalMin: min over present values; nullopt if none present
    template<class R, class Comp = std::ranges::less>
    auto OptionalMin(R&& r, Comp comp = {}) -> std::optional<OptionalRangeValueType_<R>>
    {
        using T = OptionalRangeValueType_<R>;
        std::optional<T> acc;
        for (auto&& o : r) {
            if (!o) continue;
            if (!acc || std::invoke(comp, *o, *acc)) {
                acc = *o;
            }
        }
        return acc;
    }

    // OptionalMax: max over present values; nullopt if none present
    template<class R, class Comp = std::ranges::less>
    auto OptionalMax(R&& r, Comp comp = {}) -> std::optional<OptionalRangeValueType_<R>>
    {
        using T = OptionalRangeValueType_<R>;
        std::optional<T> acc;
        for (auto&& o : r) {
            if (!o) continue;
            if (!acc || std::invoke(comp, *acc, *o)) {
                acc = *o;
            }
        }
        return acc;
    }
}