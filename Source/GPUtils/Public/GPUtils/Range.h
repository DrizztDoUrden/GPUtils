#pragma once

#include <GPUtils/Algo.h>
#include <GPUtils/Iterators.h>

#include <CoreMinimal.h>

namespace Range
{
    namespace Types
    {
        template <class TValue, TValue delta = 1>
        struct Range
        {
        private:
            struct IteratorCtorLocker {};

        public:
            using Value = TValue;

            struct ConstIterator : Iterators::ConstBase<Value, ConstIterator>
            {
            private:
                using Base = Iterators::ConstBase<Value, ConstIterator>;
                friend struct Base;

            public:
                FORCEINLINE_DEBUGGABLE ConstIterator() = default;

                FORCEINLINE_DEBUGGABLE ConstIterator(IteratorCtorLocker, Value from, Value to_)
                    : Base((from <= to) ? MoveTemp(from) : TOptional<Value>{})
                    , to(to_)
                {
                }

            protected:
                FORCEINLINE_DEBUGGABLE bool Equals(const ConstIterator& other) const
                {
                    check(this->to == other.to);
                    return this->GetValue() == other.GetValue();
                }

                FORCEINLINE_DEBUGGABLE TOptional<Value> ShiftForward()
                {
                    auto v = GetValue();
                    v += delta;
                    if (v > to)
                        return {};
                    return v;
                }

                Value to;
            };

            FORCEINLINE_DEBUGGABLE Range(Value from_, Value to_) : from(from_), to(to_) {}
            FORCEINLINE_DEBUGGABLE ConstIterator begin() const { return { IteratorCtorLocker{}, from, to, }; }
            FORCEINLINE_DEBUGGABLE ConstIterator end() const { return {}; }

        private:
            Value from;
            Value to;
        };

        template <class TTuple, std::size_t index = Algo::TupleSize<TTuple> - 1>
        struct IteratorTupleIncrement
        {
            IteratorTupleIncrement(const TTuple& start_)
                : start(start_)
            {
            }

            TTuple& operator()(TTuple& value) const
            {
                if (++value.Get<index>())
                    return value;
                value.Get<index>() = start.Get<index>();
                return IteratorTupleIncrement<TTuple, index - 1>{ start }(value);
            }

        private:
            const TTuple& start;
        };

        template <class TTuple>
        struct IteratorTupleIncrement<TTuple, 0>
        {
            IteratorTupleIncrement(const TTuple&) {}

            TTuple& operator()(TTuple& value) const
            {
                ++value.Get<0>();
                return value;
            }
        };

        template <class... TRanges>
        struct Join
        {
        private:
            using InnerIteratorPack = TTuple<typename TRanges::ConstIterator...>;
            using EndIterator = decltype(DeclVal<InnerIteratorPack>().Get<0>());
            struct IteratorCtorLocker {};

        public:
            using Value = TTuple<typename TRanges::Value...>;

            struct ConstIterator : Iterators::ConstBase<InnerIteratorPack, ConstIterator>
            {
            private:
                using Base = Iterators::ConstBase<InnerIteratorPack, ConstIterator>;
                friend struct Base;

            public:
                FORCEINLINE_DEBUGGABLE operator bool() const { return ((Base&)*this) && Base::GetValue().ApplyBefore([](auto... iterators) { return (... && iterators); }); }

                FORCEINLINE_DEBUGGABLE ConstIterator() = default;

                FORCEINLINE_DEBUGGABLE ConstIterator(IteratorCtorLocker, InnerIteratorPack from, InnerIteratorPack start_, InnerIteratorPack end_)
                    : Base(MoveTemp(from))
                    , start(MoveTemp(start_))
                    , end(end_.Get<0>())
                {
                }

            protected:
                FORCEINLINE_DEBUGGABLE Value GetValue() const
                {
                    check(*this);
                    return Base::GetValue().ApplyBefore([](auto... elements) { return MakeTuple(*elements...); });
                }

                FORCEINLINE_DEBUGGABLE bool Equals(const ConstIterator& other) const
                {
                    return Base::GetValue() == ((Base&)other).GetValue();
                }

                FORCEINLINE_DEBUGGABLE TOptional<InnerIteratorPack> ShiftForward()
                {
                    auto v = Base::GetValue();
                    if (IteratorTupleIncrement<InnerIteratorPack>(start)(v).Get<0>() == end)
                        return {};
                    return v;
                }

                InnerIteratorPack start;
                EndIterator end;
            };

            FORCEINLINE_DEBUGGABLE Join(const TRanges&... ranges) : from(ranges.begin()...), to(ranges.end()...) {}
            FORCEINLINE_DEBUGGABLE ConstIterator begin() const { return { IteratorCtorLocker{}, from, from, to, }; }
            FORCEINLINE_DEBUGGABLE ConstIterator end() const { return { IteratorCtorLocker{}, to, from, to, }; }

        private:
            InnerIteratorPack from;
            InnerIteratorPack to;
        };

        template <class TRange, class TTo>
        struct Map
        {
        private:
            struct IteratorCtorLocker {};
            using InnerIterator = typename TRange::ConstIterator;

        public:
            using Value = typename TTo;
            using Functor = TFunction<TTo(const typename TRange::Value&)>;

            struct ConstIterator : Iterators::ConstBase<InnerIterator, ConstIterator>
            {
            private:
                using Base = Iterators::ConstBase<InnerIterator, ConstIterator>;
                friend struct Base;

            public:
                FORCEINLINE_DEBUGGABLE operator bool() const { return ((Base&)*this) && Base::GetValue(); }

                FORCEINLINE_DEBUGGABLE ConstIterator() = default;

                FORCEINLINE_DEBUGGABLE ConstIterator(IteratorCtorLocker, InnerIterator&& iterator, Functor functor_)
                    : Base(MoveTemp(iterator))
                    , functor(MoveTemp(functor_))
                {
                }

            protected:
                TTo GetValue() const
                {
                    check(*this);
                    return functor(*Base::GetValue());
                }

                FORCEINLINE_DEBUGGABLE bool Equals(const ConstIterator& other) const
                {
                    return Base::GetValue() == ((Base&)other).GetValue();
                }

                FORCEINLINE_DEBUGGABLE TOptional<InnerIterator> ShiftForward()
                {
                    auto v = Base::GetValue();
                    if (!++v)
                        return {};
                    return v;
                }

                Functor functor;
            };

            FORCEINLINE_DEBUGGABLE Map(TRange range_, Functor functor_) : range(MoveTemp(range_)), functor(MoveTemp(functor_)) {}
            FORCEINLINE_DEBUGGABLE ConstIterator begin() const { return { IteratorCtorLocker{}, range.begin(), functor, }; }
            FORCEINLINE_DEBUGGABLE ConstIterator end() const { return { IteratorCtorLocker{}, range.end(), functor, }; }

        private:
            TRange range;
            Functor functor;
        };
    }

    template <class TValue, TValue delta = 1>
    auto Make(TValue from, TValue to) { return Types::Range<TValue>{ from, to, }; }

    template <class TValue, TValue delta = 1, class TSource>
    auto Indexes(const TSource& source) { return Types::Range<TValue>{ 0, source.Num() - 1, }; }

    template <class... TRanges>
    auto Join(TRanges... ranges) { return Types::Join<TRanges...>{ ranges... }; }
}

#define MakeJoinOperator(TLeft, TRight, ...) \
template<__VA_ARGS__> \
FORCEINLINE auto operator*(TLeft left, TRight right) { return Range::Types::Join<TLeft, TRight>{ left, right, }; }

#define MakeMapOperator(TRange, ...) \
template<class TFunctor, __VA_ARGS__> \
FORCEINLINE auto operator|(TRange left, TFunctor right) { return Range::Types::Map<TRange, decltype(right(*left.begin()))>{ left, right, }; }

#define X(...) __VA_ARGS__

MakeJoinOperator(X(Range::Types::Range<TLeft, lDelta>), X(Range::Types::Range<TRight, rDelta>), class TLeft, TLeft lDelta, class TRight, TRight rDelta)
MakeJoinOperator(X(Range::Types::Join<TLeft...>), X(Range::Types::Join<TRight...>), class... TLeft, class... TRight)
MakeJoinOperator(X(Range::Types::Map<TLeft, TLeftTo>), X(Range::Types::Map<TRight, TRightTo>), class TLeft, class TLeftTo, class TRight, class TRightTo)

MakeMapOperator(X(Range::Types::Range<TValue, delta>), class TValue, TValue delta)
MakeMapOperator(X(Range::Types::Join<TRanges...>), class... TRanges)
MakeMapOperator(X(Range::Types::Map<TRange, TTo>), class TRange, class TTo)

MakeJoinOperator(X(Range::Types::Join<TLeft...>), X(Range::Types::Range<TRight, rDelta>), class... TLeft, class TRight, TRight rDelta)
MakeJoinOperator(X(Range::Types::Range<TLeft, lDelta>), X(Range::Types::Join<TRight...>), class TLeft, TLeft lDelta, class... TRight)

MakeJoinOperator(X(Range::Types::Map<TLeft, TLeftTo>), X(Range::Types::Range<TRight, rDelta>), class TLeft, class TLeftTo, class TRight, TRight rDelta)
MakeJoinOperator(X(Range::Types::Range<TLeft, lDelta>), X(Range::Types::Map<TRight, TRightTo>), class TLeft, TLeft lDelta, class TRight, class TRightTo)

MakeJoinOperator(X(Range::Types::Map<TLeft, TLeftTo>), X(Range::Types::Join<TRight...>), class TLeft, class TLeftTo, class... TRight)
MakeJoinOperator(X(Range::Types::Join<TLeft...>), X(Range::Types::Map<TRight, TRightTo>), class... TLeft, class TRight, class TRightTo)

#undef MakeJoinOperator
#undef MakeMapOperator
#undef X
