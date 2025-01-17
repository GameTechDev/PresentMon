#pragma once
#include <type_traits>
#include <utility>

namespace pmon::util
{
    // deconstruct member pointer into the object type the pointer works with and the member type
    template <typename T> struct MemberPointerInfo;
    template <typename S, typename M>
    struct MemberPointerInfo<M S::*> {
        using StructType = S;
        using MemberType = M;
    };

    // get the type of the elements in any iterable type (typically containers)
    template<typename T>
    using ContainerElementType = std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>;

    // get the size of any type, even void (defined as 0 for void)
    template<typename T>
    constexpr std::size_t VoidableSizeof() {
        if constexpr (std::is_void_v<T>) {
            return 0;
        }
        else {
            return sizeof(T);
        }
    }

    // Concept to detect if a type `T` is an instantiation of a container-like template (has value_type)
    template <typename T>
    concept IsContainerLike = requires {
        typename T::value_type;
    };

    // Concept to detect if a type `T` is an instantiation of a container-like template `Template`
    template <template <typename...> typename Template, typename T>
    concept IsContainer = IsContainerLike<T> && std::is_same_v<T, Template<typename T::value_type>>;

    // trait to deduce/extract the signature details of a function by pointer
    namespace impl {
        template <typename T>
        struct FunctionPtrTraitsImpl_;
        template <typename R, typename... Args>
        struct FunctionPtrTraitsImpl_<R(*)(Args...)> {
            using ReturnType = R;
            using ParameterTypes = std::tuple<Args...>;
            template<size_t N>
            using ParameterType = std::tuple_element_t<N, ParameterTypes>;
            static constexpr size_t ParameterCount = sizeof...(Args);
        };
    }
    template <typename T>
    struct FunctionPtrTraits : impl::FunctionPtrTraitsImpl_<std::remove_cvref_t<T>> {};
}