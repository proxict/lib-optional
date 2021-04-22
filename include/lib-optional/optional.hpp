#ifndef UTILS_OPTIONAL_HPP_
#define UTILS_OPTIONAL_HPP_

#include <cassert>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

namespace libOptional {

class NullOptionalT final {
public:
    NullOptionalT() = delete;

    enum class Construct { Token };

    explicit constexpr NullOptionalT(Construct) {}
};

static constexpr NullOptionalT NullOptional(NullOptionalT::Construct::Token);

class InPlaceT final {
public:
    InPlaceT() = delete;

    enum class Construct { Token };

    explicit constexpr InPlaceT(Construct) {}
};

static constexpr InPlaceT InPlace(InPlaceT::Construct::Token);

class BadOptionalAccess : public std::exception {
public:
    BadOptionalAccess() noexcept = default;

    virtual const char* what() const noexcept override { return "Bad Optional access"; }
};

namespace swapDetail {
    using std::swap;

    template <typename T>
    void adlSwap(T& t, T& u) noexcept(noexcept(swap(t, u))) {
        swap(t, u);
    }

} // namespace swapDetail

namespace detail {

    template <bool TTest, typename TTrue, typename TFalse>
    using Conditional = typename std::conditional<TTest, TTrue, TFalse>::type;

    template <typename T>
    using IsReference = std::is_reference<T>;

    template <typename T>
    using RemoveReference = typename std::remove_reference<T>::type;

    template <typename T>
    using ReferenceWrapper = std::reference_wrapper<T>;

    template <typename T>
    using ReferenceStorage =
        Conditional<IsReference<T>::value, ReferenceWrapper<RemoveReference<T>>, RemoveReference<T>>;

    template <bool TTest, typename TType = void>
    using EnableIf = typename std::enable_if<TTest, TType>::type;

    template <bool TTest, typename TType = void>
    using DisableIf = typename std::enable_if<!TTest, TType>::type;

    class Copyable {
    public:
        Copyable() = default;
        Copyable(const Copyable&) = default;
        Copyable& operator=(const Copyable&) = default;
    };

    class Movable {
    public:
        Movable() = default;
        Movable(Movable&&) = default;
        Movable& operator=(Movable&&) = default;
    };

    class Noncopyable {
    public:
        Noncopyable() = default;
        Noncopyable(const Noncopyable&) = delete;
        Noncopyable& operator=(const Noncopyable&) = delete;
    };

    class Nonmovable {
    public:
        Nonmovable() = default;
        Nonmovable(Nonmovable&&) = delete;
        Nonmovable& operator=(Nonmovable&&) = delete;
    };

} // namespace detail

template <typename T>
class Optional final
    : protected detail::Conditional<std::is_copy_assignable<T>::value && std::is_copy_constructible<T>::value,
                                    detail::Copyable,
                                    detail::Noncopyable>,
      protected detail::Conditional<std::is_move_assignable<T>::value && std::is_move_constructible<T>::value,
                                    detail::Movable,
                                    detail::Nonmovable> {
public:
    static_assert(!std::is_rvalue_reference<T>::value, "Optional cannot be used with r-value references");
    static_assert(!std::is_same<T, NullOptionalT>::value, "Optional cannot be used with NullOptionalT");
    static_assert(!std::is_same<T, InPlaceT>::value, "Optional cannot be used with InPlaceT");

    using ValueType = detail::ReferenceStorage<T>;
    using value_type = ValueType; // std traits
    using TRaw = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    using TPtr = TRaw*;
    using TConstPtr = TRaw const*;
    using TRef = T&;
    using TConstRef = T const&;

private:
    template <typename TOther>
    static constexpr bool IsConstructibleOrConvertibleFrom() {
        return std::is_constructible<ValueType, Optional<TOther>&>::value ||
               std::is_constructible<ValueType, const Optional<TOther>&>::value ||
               std::is_constructible<ValueType, Optional<TOther>&&>::value ||
               std::is_constructible<ValueType, const Optional<TOther>&&>::value ||
               std::is_convertible<Optional<TOther>&, ValueType>::value ||
               std::is_convertible<const Optional<TOther>&, ValueType>::value ||
               std::is_convertible<Optional<TOther>&&, ValueType>::value ||
               std::is_convertible<const Optional<TOther>&&, ValueType>::value;
    }

    template <typename TOther>
    static constexpr bool IsAssignableFrom() {
        return std::is_assignable<ValueType&, Optional<TOther>&>::value ||
               std::is_assignable<ValueType&, const Optional<TOther>&>::value ||
               std::is_assignable<ValueType&, Optional<TOther>&&>::value ||
               std::is_assignable<ValueType&, const Optional<TOther>&&>::value;
    }

public:
    // Constructors
    Optional() noexcept
        : mEmpty()
        , mInitialized(false) {}

    Optional(NullOptionalT) noexcept {}

    /// Copy constructor
    ///
    /// Only available if ValueType is copy-constructible
    Optional(const Optional& other) noexcept(std::is_nothrow_constructible<ValueType>::value) {
        static_assert(std::is_copy_constructible<ValueType>::value,
                      "The underlying type of Optional must be copy-constructible");
        if (other.mInitialized) {
            construct(other.mValue);
        }
    }

    /// Move constructor
    ///
    /// Only available if ValueType is move-constructible
    Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible<ValueType>::value) {
        static_assert(std::is_move_constructible<ValueType>::value,
                      "The underlying type of Optional must be move-constructible");
        if (other.mInitialized) {
            construct(std::move(other.mValue));
            other.mInitialized = false;
        }
    }

    /// Converting copy constructor
    ///
    /// Only available if ValueType is copy-constructible
    /// \note Conditionally explicit
    template <typename TOther,
              detail::EnableIf<
                  !std::is_same<ValueType, TOther>::value &&
                      std::is_constructible<ValueType, const typename Optional<TOther>::ValueType&>::value &&
                      std::is_convertible<const typename Optional<TOther>::ValueType&, ValueType>::value &&
                      !IsConstructibleOrConvertibleFrom<TOther>(),
                  bool> = true>
    Optional(const Optional<TOther>& other) {
        if (other) {
            emplace(*other);
        }
    }

    template <typename TOther,
              detail::EnableIf<
                  !std::is_same<ValueType, TOther>::value &&
                      std::is_constructible<ValueType, const typename Optional<TOther>::ValueType&>::value &&
                      !std::is_convertible<const typename Optional<TOther>::ValueType&, ValueType>::value &&
                      !IsConstructibleOrConvertibleFrom<TOther>(),
                  bool> = false>
    explicit Optional(const Optional<TOther>& other) {
        if (other) {
            emplace(*other);
        }
    }

    /// Converting move constructor
    ///
    /// Only available if ValueType is move-constructible
    /// \note Conditionally explicit
    template <typename TOther,
              detail::EnableIf<
                  !std::is_same<ValueType, TOther>::value &&
                      std::is_constructible<ValueType, const typename Optional<TOther>::ValueType&&>::value &&
                      std::is_convertible<typename Optional<TOther>::ValueType&&, ValueType>::value &&
                      !IsConstructibleOrConvertibleFrom<TOther>(),
                  bool> = true>
    Optional(Optional<TOther>&& other) noexcept(std::is_nothrow_move_constructible<ValueType>::value) {
        if (other) {
            emplace(std::move(*other));
        }
    }

    template <typename TOther,
              detail::EnableIf<
                  !std::is_same<ValueType, TOther>::value &&
                      std::is_constructible<ValueType, const typename Optional<TOther>::ValueType&&>::value &&
                      !std::is_convertible<typename Optional<TOther>::ValueType&&, ValueType>::value &&
                      !IsConstructibleOrConvertibleFrom<TOther>(),
                  bool> = false>
    explicit Optional(Optional<TOther>&& other) noexcept(
        std::is_nothrow_move_constructible<ValueType>::value) {
        if (other) {
            emplace(std::move(*other));
        }
    }

    /// In place constructor
    template <typename... TArgs,
              typename = detail::EnableIf<std::is_constructible<ValueType, TArgs&&...>::value>>
    explicit Optional(InPlaceT,
                      TArgs&&... args) noexcept(std::is_nothrow_constructible<ValueType, TArgs...>::value) {
        construct(std::forward<TArgs>(args)...);
    }

    template <typename TOther,
              typename... TArgs,
              typename = detail::EnableIf<
                  std::is_constructible<ValueType, std::initializer_list<TOther>&, TArgs&&...>::value>>
    explicit Optional(InPlaceT,
                      std::initializer_list<TOther> list,
                      TArgs&&... args) noexcept(std::is_nothrow_constructible<ValueType, TArgs...>::value) {
        construct(list, std::forward<TArgs>(args)...);
    }

    /// Constructor
    template <
        typename TOther = ValueType,
        detail::EnableIf<std::is_constructible<ValueType, TOther&&>::value &&
                             std::is_convertible<typename Optional<TOther>::ValueType&&, ValueType>::value,
                         bool> = true>
    Optional(TOther&& value) noexcept(std::is_nothrow_constructible<ValueType, TOther&&>::value) {
        construct(std::forward<TOther>(value));
    }

    template <
        typename TOther = ValueType,
        detail::EnableIf<std::is_constructible<ValueType, TOther&&>::value &&
                             !std::is_convertible<typename Optional<TOther>::ValueType&&, ValueType>::value,
                         bool> = false>
    explicit Optional(TOther&& value) noexcept(std::is_nothrow_constructible<ValueType, TOther&&>::value) {
        construct(std::forward<TOther>(value));
    }

    // Destructor
    ~Optional() noexcept {
        if (mInitialized) {
            reset();
        }
    }

    // Assignment operators
    Optional& operator=(NullOptionalT) noexcept {
        reset();
        return *this;
    }

    Optional&
    operator=(const Optional& other) noexcept(std::is_nothrow_assignable<ValueType, ValueType>::value) {
        static_assert(std::is_copy_assignable<ValueType>::value &&
                          std::is_copy_constructible<ValueType>::value,
                      "The underlying type of Optional must be copy-constructible and copy-assignable");
        if (mInitialized && other.mInitialized) {
            mValue = other.mValue;
        } else {
            if (other.mInitialized) {
                construct(other.mValue);
            } else {
                reset();
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept(std::is_nothrow_assignable<ValueType, ValueType>::value) {
        static_assert(std::is_move_assignable<ValueType>::value &&
                          std::is_move_constructible<ValueType>::value,
                      "The underlying type of Optional must be move-constructible and move-assignable");
        if (mInitialized && other.mInitialized) {
            mValue = std::move(other.mValue);
            other.mInitialized = false;
        } else {
            if (other.mInitialized) {
                construct(std::move(other.mValue));
                other.mInitialized = false;
            } else {
                reset();
            }
        }
        return *this;
    }

    template <typename TOther = ValueType>
    detail::EnableIf<!std::is_same<Optional<TRaw>, typename std::decay<TOther>::type>::value &&
                         std::is_constructible<ValueType, TOther>::value &&
                         !(std::is_scalar<ValueType>::value &&
                           std::is_same<ValueType, typename std::decay<TOther>::type>::value) &&
                         std::is_assignable<ValueType&, TOther>::value,
                     Optional&>
    operator=(TOther&& value) noexcept(std::is_nothrow_constructible<ValueType, TOther>::value) {
        if (mInitialized) {
            mValue = std::forward<TOther>(value);
        } else {
            construct(std::forward<TOther>(value));
        }
        return *this;
    }

    template <typename TOther>
    detail::EnableIf<!std::is_same<TOther, TRaw>::value &&
                         std::is_constructible<ValueType, const TOther&>::value &&
                         std::is_assignable<ValueType&, TOther>::value &&
                         !IsConstructibleOrConvertibleFrom<TOther>() && !IsAssignableFrom<TOther>(),
                     Optional&>
    operator=(const Optional<TOther>& other) {
        if (mInitialized && other.mInitialized) {
            mValue = other.mValue;
        } else {
            if (other.mInitialized) {
                construct(other.mValue);
            } else {
                reset();
            }
        }
        return *this;
    }

    template <typename TOther>
    detail::EnableIf<!std::is_same<TOther, TRaw>::value && std::is_constructible<ValueType, TOther>::value &&
                         std::is_assignable<ValueType&, TOther>::value &&
                         !IsConstructibleOrConvertibleFrom<TOther>() && !IsAssignableFrom<TOther>(),
                     Optional&>
    operator=(Optional<TOther>&& other) noexcept(std::is_nothrow_move_constructible<ValueType>::value&&
                                                     std::is_nothrow_move_assignable<ValueType>::value) {
        if (mInitialized && other.mInitialized) {
            mValue = std::move(other.mValue);
            other.mInitialized = false;
        } else {
            if (other.mInitialized) {
                construct(std::move(other.mValue));
                other.mInitialized = false;
            } else {
                reset();
            }
        }
        return *this;
    }

    // Modifiers
    template <typename... TArgs,
              typename = detail::EnableIf<!detail::IsReference<T>::value &&
                                          std::is_constructible<ValueType, TArgs...>::value>>
    ValueType& emplace(TArgs&&... args) noexcept(std::is_nothrow_constructible<ValueType, TArgs...>::value) {
        reset();
        construct(std::forward<TArgs>(args)...);
        return mValue;
    }

    template <typename TOther,
              typename... TArgs,
              typename = detail::EnableIf<
                  !detail::IsReference<T>::value &&
                  std::is_constructible<ValueType, std::initializer_list<TOther>&, TArgs&&...>::value>>
    ValueType& emplace(std::initializer_list<TOther> list,
                       TArgs&&... args) noexcept(std::is_nothrow_constructible<ValueType, TArgs...>::value) {
        reset();
        construct(list, std::forward<TArgs>(args)...);
        return mValue;
    }

    void swap(Optional& other) noexcept(std::is_nothrow_move_constructible<ValueType>::value&& noexcept(
        swapDetail::adlSwap(std::declval<T&>(), std::declval<T&>()))) {
        if (mInitialized && other.mInitialized) {
            using std::swap;
            swap(mValue, other.mValue);
        } else if (mInitialized) {
            other.construct(std::move(mValue));
            reset();
        } else if (other.mInitialized) {
            construct(std::move(other.mValue));
            other.reset();
        }
    }

    void reset() noexcept {
        if (mInitialized) {
            mInitialized = false;
            mValue.~ValueType();
        }
    }

    // Observers
    explicit operator bool() const noexcept { return mInitialized; }

    bool operator!() const noexcept { return !mInitialized; }

    bool hasValue() const noexcept { return mInitialized; }

    TConstPtr operator->() const noexcept {
        assert(mInitialized);
        return &mValue;
    }

    TPtr operator->() noexcept {
        assert(mInitialized);
        return &mValue;
    }

    TConstRef operator*() const& noexcept {
        assert(mInitialized);
        return mValue;
    }

    TRef operator*() & noexcept {
        assert(mInitialized);
        return mValue;
    }

    template <typename TOther = T,
              typename = detail::EnableIf<!detail::IsReference<TOther>::value, ValueType&&>>
    ValueType&& operator*() && noexcept {
        assert(mInitialized);
        return std::move(mValue);
    }

    template <typename TOther = T, typename detail::EnableIf<!detail::IsReference<TOther>::value, bool> = 0>
    const ValueType& value() const& noexcept(false) {
        if (!mInitialized) {
            throw BadOptionalAccess();
        }
        return mValue;
    }

    template <typename TOther = T, typename detail::EnableIf<detail::IsReference<TOther>::value, bool> = 1>
    TConstRef value() const& noexcept(false) {
        if (!mInitialized) {
            throw BadOptionalAccess();
        }
        return mValue.get();
    }

    template <typename TOther = T, typename detail::EnableIf<!detail::IsReference<TOther>::value, bool> = 0>
    ValueType& value() & noexcept(false) {
        if (!mInitialized) {
            throw BadOptionalAccess();
        }
        return mValue;
    }

    template <typename TOther = T, typename detail::EnableIf<detail::IsReference<TOther>::value, bool> = 1>
    TRef value() & noexcept(false) {
        if (!mInitialized) {
            throw BadOptionalAccess();
        }
        return mValue.get();
    }

    template <typename TOther = T, typename = detail::EnableIf<!detail::IsReference<TOther>::value>>
    ValueType&& value() && noexcept(false) {
        if (!mInitialized) {
            throw BadOptionalAccess();
        }
        return std::move(mValue);
    }

    template <typename TOther,
              detail::EnableIf<std::is_same<TOther, T>::value && !detail::IsReference<T>::value &&
                                   std::is_constructible<TRaw, TOther>::value,
                               int> = 0>
    auto valueOr(TOther&& value) const noexcept(std::is_nothrow_constructible<TRaw, TOther>::value)
        -> detail::EnableIf<std::is_constructible<TRaw, TOther>::value, TRaw> {
        return mInitialized ? mValue : static_cast<TRaw>(std::forward<TOther>(value));
    }

    template <typename TOther,
              detail::EnableIf<!std::is_same<const TOther&, T>::value && std::is_same<TOther&, T>::value &&
                                   detail::IsReference<T>::value,
                               int> = 1>
    TRef valueOr(TOther& value) noexcept(std::is_nothrow_constructible<TRaw, TOther>::value) {
        return mInitialized ? mValue.get() : value;
    }

    template <typename TOther,
              detail::EnableIf<!std::is_same<TOther&, T>::value && std::is_same<const TOther&, T>::value &&
                                   detail::IsReference<T>::value,
                               int> = 2>
    TConstRef valueOr(const TOther& value) const
        noexcept(std::is_nothrow_constructible<TRaw, TOther>::value) {
        return mInitialized ? static_cast<const TRaw&>(mValue.get()) : value;
    }

private:
    template <typename... TArgs>
    void construct(TArgs&&... args) noexcept(std::is_nothrow_constructible<ValueType, TArgs...>::value) {
        new ((void*)&mValue) ValueType(std::forward<TArgs>(args)...);
        mInitialized = true;
    }

    // Just so ValueType doesn't have to be default-constructible
    struct Empty {};
    union {
        Empty mEmpty;
        ValueType mValue;
    };
    bool mInitialized = false;

    template <typename TValueOther>
    friend class Optional;
};

// Compare Optional<T> to Optional<T>
template <typename T>
constexpr bool operator==(const Optional<T>& x, const Optional<T>& y) {
    return bool(x) != bool(y) ? false : bool(x) == false ? true : *x == *y;
}

template <typename T>
constexpr bool operator!=(const Optional<T>& x, const Optional<T>& y) {
    return !(x == y);
}

template <typename T>
constexpr bool operator<(const Optional<T>& x, const Optional<T>& y) {
    return (!y) ? false : (!x) ? true : *x < *y;
}

template <typename T>
constexpr bool operator>(const Optional<T>& x, const Optional<T>& y) {
    return (y < x);
}

template <typename T>
constexpr bool operator<=(const Optional<T>& x, const Optional<T>& y) {
    return !(y < x);
}

template <typename T>
constexpr bool operator>=(const Optional<T>& x, const Optional<T>& y) {
    return !(x < y);
}

// Compare Optional<T> to T
template <typename T>
constexpr bool operator==(const Optional<T>& x, const T& v) {
    return bool(x) ? *x == v : false;
}

template <typename T>
constexpr bool operator==(const T& v, const Optional<T>& x) {
    return bool(x) ? v == *x : false;
}

template <typename T>
constexpr bool operator!=(const Optional<T>& x, const T& v) {
    return bool(x) ? *x != v : true;
}

template <typename T>
constexpr bool operator!=(const T& v, const Optional<T>& x) {
    return bool(x) ? v != *x : true;
}

template <typename T>
constexpr bool operator<(const Optional<T>& x, const T& v) {
    return bool(x) ? *x < v : true;
}

template <typename T>
constexpr bool operator>(const T& v, const Optional<T>& x) {
    return bool(x) ? v > *x : true;
}

template <typename T>
constexpr bool operator>(const Optional<T>& x, const T& v) {
    return bool(x) ? *x > v : false;
}

template <typename T>
constexpr bool operator<(const T& v, const Optional<T>& x) {
    return bool(x) ? v < *x : false;
}

template <typename T>
constexpr bool operator>=(const Optional<T>& x, const T& v) {
    return bool(x) ? *x >= v : false;
}

template <typename T>
constexpr bool operator<=(const T& v, const Optional<T>& x) {
    return bool(x) ? v <= *x : false;
}

template <typename T>
constexpr bool operator<=(const Optional<T>& x, const T& v) {
    return bool(x) ? *x <= v : true;
}

template <typename T>
constexpr bool operator>=(const T& v, const Optional<T>& x) {
    return bool(x) ? v >= *x : true;
}

// Compare Optional<T> to NullOptionalT
template <typename T>
constexpr bool operator==(const Optional<T>& x, NullOptionalT) noexcept {
    return (!x);
}

template <typename T>
constexpr bool operator==(NullOptionalT, const Optional<T>& x) noexcept {
    return (!x);
}

template <typename T>
constexpr bool operator!=(const Optional<T>& x, NullOptionalT) noexcept {
    return bool(x);
}

template <typename T>
constexpr bool operator!=(NullOptionalT, const Optional<T>& x) noexcept {
    return bool(x);
}

template <typename T>
constexpr bool operator<(const Optional<T>&, NullOptionalT) noexcept {
    return false;
}

template <typename T>
constexpr bool operator<(NullOptionalT, const Optional<T>& x) noexcept {
    return bool(x);
}

template <typename T>
constexpr bool operator<=(const Optional<T>& x, NullOptionalT) noexcept {
    return (!x);
}

template <typename T>
constexpr bool operator<=(NullOptionalT, const Optional<T>&) noexcept {
    return true;
}

template <typename T>
constexpr bool operator>(const Optional<T>& x, NullOptionalT) noexcept {
    return bool(x);
}

template <typename T>
constexpr bool operator>(NullOptionalT, const Optional<T>&) noexcept {
    return false;
}

template <typename T>
constexpr bool operator>=(const Optional<T>&, NullOptionalT) noexcept {
    return true;
}

template <typename T>
constexpr bool operator>=(NullOptionalT, const Optional<T>& x) noexcept {
    return (!x);
}

} // namespace libOptional

namespace std {

template <typename T>
struct hash<libOptional::Optional<T>> {
    using argument_type = libOptional::Optional<T>;
    using result_type = std::size_t;

    result_type operator()(const argument_type& o) const noexcept(noexcept(hash<T>{}(*o))) {
        return o.hasValue() ? std::hash<T>{}(o.value()) : -23; // random magic number
    }
};

template <typename T>
void swap(libOptional::Optional<T>& lhs, libOptional::Optional<T>& rhs) {
    lhs.swap(rhs);
}

} // namespace std

#endif // UTILS_OPTIONAL_HPP_
