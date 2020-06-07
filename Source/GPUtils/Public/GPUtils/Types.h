#pragma once

#include <CoreMinimal.h>
#include <Templates/EnableIf.h>
#include <Templates/IsPointer.h>

namespace Types
{
    template <bool condition, class TType>
    using EnableIf = typename TEnableIf<condition, TType>::Type;

    template <class TType>
    constexpr bool IsPointer = TIsPointer<TType>::Value;

    namespace Detail
    {
        template <std::size_t id, class TFirst, class... TRest>
        struct Get : Get<id - 1, TRest...>
        {
        };

        template <class TType>
        struct Get<0, TType>
        {
            using Type = TType;
        };
    }

    template <std::size_t id, class... TTypes>
    using Get = typename Detail::Get<id, TTypes...>::Type;

    template <class TType0, class TType1, class... TRest>
    constexpr bool AreSame = AreSame<TType0, TType1> && AreSame<TType0, TRest...>;

    template <class TType0>
    constexpr bool AreSame<TType0, TType0> = true;

    template <class TType0, class TType1>
    constexpr bool AreSame<TType0, TType1> = false;

    template <class TValue, TValue... values>
    struct ValueSequence {};

    template<size_t... values>
    using IndexSequence = ValueSequence<std::size_t, values...>;

    namespace Detail
    {
        template <std::size_t index, std::size_t... rest>
        struct MakeIndexSequence : public MakeIndexSequence<index - 1U, index - 1U, rest...>
        { };

        template <std::size_t... indices>
        struct MakeIndexSequence<0U, indices... >
        {
            using Type = IndexSequence<indices... >;
        };
    }

    template <std::size_t count>
    using MakeIndexSequence = typename Detail::MakeIndexSequence<count>::Type;
}
