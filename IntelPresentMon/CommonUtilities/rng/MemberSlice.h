#pragma once
#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>
#include "../Meta.h"

namespace pmon::util::rng
{
    // Takes a range R and a pointer-to-data-member `mp`.
    // If range elements have that member, returns a view of it.
    // If range elements are pair-like with `.second` having that member,
    // it returns a view of `.second.*mp`.
    template<std::ranges::input_range R, class MP>
        requires std::is_member_object_pointer_v<std::remove_cvref_t<MP>>
    auto MemberSlice(R&& r, MP mp)
    {
        // Transform projection that preserves reference categories and cv
        auto proj = [mp]<class E>(E&& e) -> decltype(auto) {
            // Direct member on the element
            if constexpr (requires { (std::forward<E>(e)).*mp; }) {
                return (std::forward<E>(e)).*mp;
            }
            // Member on mapped value of a pair-like element
            else if constexpr (requires { (std::forward<E>(e)).second.*mp; }) {
                return (std::forward<E>(e)).second.*mp;
            }
            else {
                static_assert(DependentFalse<E>, "MemberSlice: element type does not match the member pointer");
            }
        };
        return std::forward<R>(r) | std::views::transform(proj);
    }
}
