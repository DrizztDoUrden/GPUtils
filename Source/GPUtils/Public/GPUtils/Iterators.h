#pragma once

#include <GPUtils/Types.h>

#include <CoreMinimal.h>

template <class TType>
FORCEINLINE TType ValidateType(TType value)
{
    static_assert(TIsSame<decltype(value), TType>::Value, "Unexpected type");
    return value;
}

namespace Iterators
{
    template <class TValue, class TDerived>
    struct Base;

    template <class TValue, class TDerived>
    struct ConstBase
    {
    public:
        FORCEINLINE_DEBUGGABLE operator bool() const { return value.IsSet(); }

        FORCEINLINE_DEBUGGABLE bool operator ==(const TDerived& other) const
        {
            if (!AsDerieved() && !other) return true;
            if (!AsDerieved() || !other) return false;
            return AsDerieved().Equals(other);
        }

        FORCEINLINE_DEBUGGABLE auto operator*() const { check(AsDerieved()); return AsDerieved().GetValue(); }

        template<class TValue2 = decltype(DeclVal<TDerived>().GetValue())>
        FORCEINLINE_DEBUGGABLE Types::EnableIf<!Types::IsPointer<TValue2>, const TValue2*> operator->() const { check(AsDerieved()); return &AsDerieved().GetValue(); }

        FORCEINLINE_DEBUGGABLE TDerived& operator++()
        {
            check(AsDerieved());
            value = ValidateType<TOptional<TValue>>(AsDerieved().ShiftForward());
            return AsDerieved();
        }

        FORCEINLINE_DEBUGGABLE TDerived operator++(int)
        {
            check(AsDerieved());
            const auto ret = AsDerieved();
            ++(*this);
            return ret;
        }

        FORCEINLINE_DEBUGGABLE TDerived operator+(std::size_t shift) const
        {
            check(AsDerieved());
            const auto ret = AsDerieved();
            for (auto i = 0; i < shift; ++i)
                ++ret;
            return ret;
        }

        FORCEINLINE_DEBUGGABLE const TValue& GetValue() const
        {
            return value.GetValue();
        }

    protected:
        ConstBase() = default;
        FORCEINLINE_DEBUGGABLE ConstBase(TOptional<TValue>&& value_) : value(value_)
        {
            if (!AsDerieved())
                value.Reset();
        }

    private:
        TOptional<TValue> value;

        TDerived& AsDerieved() { return static_cast<TDerived&>(*this); }
        const TDerived& AsDerieved() const { return static_cast<const TDerived&>(*this); }

        friend struct Base<TValue, TDerived>;
    };

    template <class TValue, class TDerived>
    struct Base : ConstBase<TValue, TDerived>
    {
    private:
        using TBase = ConstBase<TValue, TDerived>;

    public:
        FORCEINLINE_DEBUGGABLE TValue& operator*() { check(AsDerieved()); return GetValue(); }

        template<class TValue2 = decltype(DeclVal<TDerived>().GetValue())>
        FORCEINLINE_DEBUGGABLE Types::EnableIf<!Types::IsPointer<TValue2>, TValue*> operator->() { check(AsDerieved()); return &AsDerieved().GetValue(); }

        FORCEINLINE_DEBUGGABLE TValue& GetValue()
        {
            return value.GetValue();
        }

    protected:
        Base() = default;
        FORCEINLINE_DEBUGGABLE Base(TOptional<TValue>&& value_) : TBase(MoveTemp(value_)) {}

    private:
        TDerived& AsDerieved() { return *reinterpret_cast<TDerived*>(this); }
        const TDerived& AsDerieved() const { return *reinterpret_cast<const TDerived*>(this); }
    };

    template <class TValue, class TDerived>
    struct ConstBidirBase : ConstBase<TValue, TDerived>
    {
    private:
        using Base = ConstBase<TValue, TDerived>;

    public:
        FORCEINLINE_DEBUGGABLE TDerived& operator--()
        {
            check(AsDerieved());
            value = ValidateType<TOptional<TValue>>(AsDerieved().ShiftBack());
            return AsDerieved();
        }

        FORCEINLINE_DEBUGGABLE TDerived operator--(int)
        {
            check(AsDerieved());
            const auto ret = AsDerieved();
            --(*this);
            return ret;
        }

    protected:
        ConstBidirBase() = default;
        FORCEINLINE_DEBUGGABLE ConstBidirBase(TOptional<TValue>&& value_) : Base(MoveTemp(value_)) {}

    private:
        TDerived& AsDerieved() { return *reinterpret_cast<TDerived*>(this); }
        const TDerived& AsDerieved() const { return *reinterpret_cast<const TDerived*>(this); }
    };

    template <class TValue, class TDerived>
    struct BidirBase : Base<TValue, TDerived>
    {
    private:
        using TBase = ConstBase<TValue, TDerived>;

    public:
        FORCEINLINE_DEBUGGABLE TDerived& operator--()
        {
            check(AsDerieved());
            value = ValidateType<TOptional<TValue>>(AsDerieved().ShiftBack());
            return AsDerieved();
        }

        FORCEINLINE_DEBUGGABLE TDerived operator--(int)
        {
            check(AsDerieved());
            const auto ret = AsDerieved();
            --(*this);
            return ret;
        }

    protected:
        BidirBase() = default;
        FORCEINLINE_DEBUGGABLE BidirBase(TOptional<TValue>&& value_) : TBase(MoveTemp(value_)) {}

    private:
        TDerived& AsDerieved() { return *reinterpret_cast<TDerived*>(this); }
        const TDerived& AsDerieved() const { return *reinterpret_cast<const TDerived*>(this); }
    };
}
