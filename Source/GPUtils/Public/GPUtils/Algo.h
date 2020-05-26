#pragma once

#include <GPUtils/Types.h>

#include <Templates/Tuple.h>

namespace Algo
{
    template <class TTuple>
    constexpr std::size_t TupleSize = TTupleArity<TTuple>::Value;

    template <class TTo>
    auto FromTuple() { return [](auto tuple) { return tuple.ApplyBefore([](auto... elements) { return TTo{ elements... }; }); }; }

    template <class TTo, std::size_t first, std::size_t... indices>
    auto FromTuple() { return [](auto tuple) { return TTo{ tuple.Get<first>(), tuple.Get<indices>()... }; }; }

    template <class TTo>
    auto CastTuple() { return [](auto tuple) { return tuple.ApplyBefore([](auto... elements) { return MakeTuple(static_cast<TTo>(elements)...); }); }; }

    template <class TTo>
    auto Cast() { return [](auto in) { return static_cast<TTo>(in); }; }
}
