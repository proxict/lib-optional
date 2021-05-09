#include "lib-optional/optional.hpp"

#include <gmock/gmock.h>
#include <sstream>
#include <string>

using namespace libOptional;

struct A {};
struct B : A {};

class OptionalTestAware : public ::testing::Test {
protected:
    struct State {
        bool ctor;
        bool dtor;
        bool copyAssigned;
        bool copyCtor;
        bool moveAssigned;
        bool moveCtor;

        State& operator=(bool state) {
            ctor = state;
            dtor = state;
            copyAssigned = state;
            copyCtor = state;
            moveAssigned = state;
            moveCtor = state;
            return *this;
        }

        bool operator==(const bool v) const {
            return ctor == v && dtor == v && copyAssigned == v && copyCtor == v && moveAssigned == v &&
                   moveCtor == v;
        }

        operator bool() const { return *this == true; }

        State() { *this = false; }
    };

    class Aware final {
    public:
        Aware(State& state)
            : mState(state) {
            state.ctor = true;
        }

        ~Aware() { mState.dtor = true; }

        Aware(const Aware& other)
            : mState(other.mState) {
            mState.copyCtor = true;
        }

        Aware(Aware&& other)
            : mState(other.mState) {
            mState.moveCtor = true;
        }

        Aware& operator=(const Aware&) {
            mState.copyAssigned = true;
            return *this;
        }

        Aware& operator=(Aware&&) {
            mState.moveAssigned = true;
            return *this;
        }

    private:
        State& mState;
    };

    virtual void SetUp() override { mState = false; }

    virtual void TearDown() override { mState = false; }

    State& getState() { return mState; }

    const State& getState() const { return mState; }

private:
    State mState;
};

template <typename TWhat, typename TFrom>
class IsImplicitlyConstructibleFrom {
    static std::false_type test(...);

    static std::true_type test(TWhat);

public:
    static constexpr bool value = decltype(test(std::declval<TFrom>()))::value;
};

TEST_F(OptionalTestAware, NullOpt) {
    {
        Optional<Aware> a(NullOptional);
        EXPECT_TRUE(!a);
        EXPECT_FALSE(bool(a));
    }
    EXPECT_FALSE(getState());
}

TEST_F(OptionalTestAware, Default) {
    { Optional<Aware> a; }
    EXPECT_FALSE(getState());
}

TEST_F(OptionalTestAware, value) {
    { Optional<Aware> a(getState()); }
    EXPECT_TRUE(getState().ctor);
    EXPECT_TRUE(getState().dtor);
}

TEST_F(OptionalTestAware, copyCtor) {
    Optional<Aware> a(getState());
    EXPECT_TRUE(bool(a));
    EXPECT_FALSE(getState().copyCtor);
    EXPECT_FALSE(getState().copyAssigned);
    getState() = false;

    Optional<Aware> b(a);
    EXPECT_FALSE(getState().ctor);
    EXPECT_FALSE(getState().copyAssigned);
    EXPECT_TRUE(getState().copyCtor);
    getState() = false;

    Optional<Aware> u;
    Optional<Aware> c(u);
    EXPECT_FALSE(getState().ctor);
    EXPECT_FALSE(getState().copyAssigned);
    EXPECT_FALSE(getState().copyCtor);

    // Optional<Noncopyable> c;
    // Optional<Noncopyable> d(c); // Fails to compile - OK
}

TEST_F(OptionalTestAware, moveCtor) {
    Optional<Aware> a(getState());
    EXPECT_TRUE(bool(a));
    EXPECT_FALSE(getState().copyCtor);
    getState() = false;

    Optional<Aware> b(std::move(a));
    EXPECT_FALSE(getState().ctor);
    EXPECT_FALSE(getState().copyCtor);
    EXPECT_FALSE(getState().copyAssigned);
    EXPECT_FALSE(getState().moveAssigned);
    EXPECT_TRUE(getState().moveCtor);
    getState() = false;

    Optional<Aware> u;
    Optional<Aware> c(std::move(u));
    EXPECT_FALSE(getState().ctor);
    EXPECT_FALSE(getState().copyCtor);
    EXPECT_FALSE(getState().copyAssigned);
    EXPECT_FALSE(getState().moveAssigned);
    EXPECT_FALSE(getState().moveCtor);
}

TEST_F(OptionalTestAware, copyAssign) {
    { // Assigning an initialized object to an already initialized one
        // so we should observe a copy assignment being called.
        Optional<Aware> a(getState());
        Optional<Aware> b(getState());
        b = a;
        EXPECT_FALSE(getState().copyCtor);
        EXPECT_TRUE(getState().copyAssigned);
    }
    getState() = false;
    { // Assigning an initialized object to an uninitialized
        // so we should observe a copy ctor being called.
        Optional<Aware> a(getState());
        Optional<Aware> b;
        b = a;
        EXPECT_FALSE(getState().copyAssigned);
        EXPECT_TRUE(getState().copyCtor);
    }
    getState() = false;
    { // Assigning an uninitialized object to an uninitialized one
        // so we should observe a dtor being called
        Optional<Aware> a;
        Optional<Aware> b(getState());
        b = a;
        EXPECT_TRUE(getState().ctor);
        EXPECT_TRUE(getState().dtor);
    }
    {
        Optional<A*> a(nullptr);
        Optional<B*> b(nullptr);
        a = b;
        EXPECT_TRUE(a);
        EXPECT_TRUE(b);
    }
    {
        Optional<A*> a;
        Optional<B*> b;
        a = b;
        EXPECT_FALSE(a);
        EXPECT_FALSE(b);
    }
    {
        Optional<A*> a(nullptr);
        Optional<B*> b;
        a = b;
        EXPECT_FALSE(a);
        EXPECT_FALSE(b);
    }
    {
        Optional<A*> a;
        Optional<B*> b(nullptr);
        a = b;
        EXPECT_TRUE(a);
        EXPECT_TRUE(b);
    }
}

TEST_F(OptionalTestAware, moveAssign) {
    { // Assigning an initialized object to an already initialized one
        // so we should observe a move assignment being called.
        Optional<Aware> a(getState());
        Optional<Aware> b(getState());
        b = std::move(a);
        EXPECT_FALSE(getState().copyCtor);
        EXPECT_FALSE(getState().moveCtor);
        EXPECT_FALSE(getState().copyAssigned);
        EXPECT_TRUE(getState().moveAssigned);
    }
    getState() = false;
    { // Assigning an initialized object to an uninitialized
        // so we should observe a move ctor being called.
        Optional<Aware> a(getState());
        Optional<Aware> b;
        b = std::move(a);
        EXPECT_FALSE(getState().copyCtor);
        EXPECT_FALSE(getState().copyAssigned);
        EXPECT_FALSE(getState().moveAssigned);
        EXPECT_TRUE(getState().moveCtor);
    }
    getState() = false;
    { // Assigning an uninitialized object to an uninitialized one
        // so we should observe a dtor being called
        Optional<Aware> a;
        Optional<Aware> b(getState());
        b = std::move(a);
        EXPECT_TRUE(getState().ctor);
        EXPECT_TRUE(getState().dtor);
    }
    {
        Optional<std::string> a("dog");
        std::string b;
        a = std::move(b);
        EXPECT_TRUE(bool(a));
        EXPECT_EQ(a->size(), 0);
    }
    {
        Optional<std::string> a;
        std::string b("dog");
        a = std::move(b);
        EXPECT_TRUE(bool(a));
        EXPECT_EQ(*a, "dog");
    }
    {
        Optional<A*> a(nullptr);
        Optional<B*> b(nullptr);
        a = std::move(b);
        EXPECT_TRUE(a);
    }
    {
        Optional<A*> a;
        Optional<B*> b;
        a = std::move(b);
        EXPECT_FALSE(a);
    }
    {
        Optional<A*> a(nullptr);
        Optional<B*> b;
        a = std::move(b);
        EXPECT_FALSE(a);
    }
    {
        Optional<A*> a;
        Optional<B*> b(nullptr);
        a = std::move(b);
        EXPECT_TRUE(a);
    }
}

TEST(OptionalTest, convertingCopyCtor) {
    {
        Optional<std::string> a("This is a test string");
        Optional<std::ostringstream> b(a);
        EXPECT_TRUE(bool(a));
        EXPECT_TRUE(bool(b));
        EXPECT_EQ(*a, b->str());
    }
    {
        Optional<std::string> a;
        Optional<std::ostringstream> b(a);
        EXPECT_FALSE(bool(a));
        EXPECT_FALSE(bool(b));
    }
    {
        Optional<B*> a(nullptr);
        Optional<A*> b(a);
        EXPECT_TRUE(bool(a));
        EXPECT_TRUE(bool(b));
    }
    {
        Optional<B*> a;
        Optional<A*> b(a);
        EXPECT_FALSE(bool(a));
        EXPECT_FALSE(bool(b));
    }
}

TEST(OptionalTest, convertingMoveCtor) {
    {
        Optional<std::string> a("This is a test string");
        Optional<std::ostringstream> b(std::move(a));
        EXPECT_TRUE(bool(a));
        EXPECT_TRUE(bool(b));
        EXPECT_EQ(*a, b->str());
    }
    {
        Optional<std::string> a;
        Optional<std::ostringstream> b(std::move(a));
        EXPECT_FALSE(bool(a));
        EXPECT_FALSE(bool(b));
    }
    {
        Optional<B*> a(nullptr);
        Optional<A*> b(std::move(a));
        EXPECT_TRUE(bool(a));
        EXPECT_TRUE(bool(b));
    }
    {
        Optional<B*> a;
        Optional<A*> b(std::move(a));
        EXPECT_FALSE(bool(a));
        EXPECT_FALSE(bool(b));
    }
}

TEST(OptionalTest, inPlaceCtor) {
    struct S {
        S(int a, int b, std::string str)
            : a(a)
            , b(b)
            , str(std::move(str)) {}

        S(const S&) = delete;
        S& operator=(const S&) = delete;
        S(S&&) = delete;
        S& operator=(S&&) = delete;

        int a, b;
        std::string str;
    };
    Optional<S> v(InPlace, 1, 2, "InPlace");
    EXPECT_TRUE(bool(v));
    EXPECT_EQ(v->a, 1);
    EXPECT_EQ(v->b, 2);
    EXPECT_EQ(v->str, "InPlace");
}

TEST(OptionalTest, inPlaceInitializerList) {
    Optional<std::string> v(InPlace, { 'a', 'b', 'c' });
    EXPECT_TRUE(bool(v));
    EXPECT_EQ(*v, "abc");
}

TEST(OptionalTest, ctorValue) {
    std::string s("Psycho");
    Optional<std::string> v(s);
    EXPECT_TRUE(bool(v));
    EXPECT_EQ(v, s);
}

TEST(OptionalTest, ctorValueMove) {
    std::string s("Psycho");
    Optional<std::string> v(std::move(s));
    EXPECT_TRUE(bool(v));
    EXPECT_EQ(v, std::string("Psycho"));
    EXPECT_TRUE(s.empty());
}

TEST_F(OptionalTestAware, assignNullOpt) {
    Optional<Aware> a(getState());
    EXPECT_TRUE(bool(a));
    EXPECT_TRUE(getState().ctor);
    EXPECT_FALSE(getState().dtor);
    a = NullOptional;
    EXPECT_FALSE(bool(a));
    EXPECT_TRUE(getState().dtor);
}

TEST_F(OptionalTestAware, emplace) {
    {
        {
            Optional<Aware> a;
            EXPECT_FALSE(bool(a));
            a.emplace(getState());
            EXPECT_TRUE(bool(a));
            EXPECT_TRUE(getState().ctor);
            EXPECT_FALSE(getState().copyCtor);
            EXPECT_FALSE(getState().moveCtor);
            EXPECT_FALSE(getState().copyAssigned);
            EXPECT_FALSE(getState().moveAssigned);
        }
        EXPECT_TRUE(getState().dtor);
    }
    {
        Optional<std::vector<int>> a;
        a.emplace({ 1, 2, 3 });
        EXPECT_TRUE(bool(a));
        EXPECT_EQ(*a, std::vector<int>({ 1, 2, 3 }));
    }
}

TEST_F(OptionalTestAware, reset) {
    Optional<Aware> a(getState());
    EXPECT_TRUE(bool(a));
    a.reset();
    EXPECT_FALSE(bool(a));
    EXPECT_TRUE(getState().dtor);
}

TEST_F(OptionalTestAware, swap) {
    {
        Optional<std::string> a("A");
        Optional<std::string> b("B");
        std::swap(a, b);
        EXPECT_TRUE(bool(a));
        EXPECT_TRUE(bool(b));

        EXPECT_EQ(*a, "B");
        EXPECT_EQ(*b, "A");
    }
    {
        Optional<std::string> a("A");
        Optional<std::string> b;
        std::swap(a, b);
        EXPECT_FALSE(bool(a));
        EXPECT_TRUE(bool(b));

        EXPECT_EQ(a, NullOptional);
        EXPECT_EQ(*b, "A");
    }
    {
        Optional<std::string> a;
        Optional<std::string> b("B");
        std::swap(a, b);
        EXPECT_TRUE(bool(a));
        EXPECT_FALSE(bool(b));

        EXPECT_EQ(*a, "B");
        EXPECT_EQ(b, NullOptional);
    }
}

TEST(OptionalTest, reset) {
    {
        Optional<A> o(A{});
        o.reset();
        EXPECT_FALSE(bool(o));
    }
    {
        A v;
        Optional<const A&> o(v);
        o.reset();
        EXPECT_FALSE(bool(o));
    }
    {
        A v;
        Optional<A&> o(v);
        o.reset();
        EXPECT_FALSE(bool(o));
    }
    {
        Optional<unsigned long> o(1);
        o.reset();
        EXPECT_FALSE(bool(o));
    }
}

TEST(OptionalTest, assignViaRef) {
    Optional<int> v(42);
    EXPECT_EQ(*v, 42);
    *v = 0;
    EXPECT_EQ(*v, 0);
}

TEST(OptionalTest, dereference) {
    {
        const Optional<int> v(1);
        EXPECT_EQ(*v, 1);
    }
    {
        int k = 1;
        const Optional<int&> v(k);
        EXPECT_EQ(*v, 1);
    }
    {
        const Optional<std::string> v("abc");
        EXPECT_EQ(v->size(), 3);
    }
    {
        std::string k("abc");
        const Optional<std::string&> v(k);
        EXPECT_EQ(v->size(), 3);
    }
    {
        std::string k("abc");
        Optional<std::string&> v(k);
        EXPECT_EQ(v->size(), 3);
    }
}

TEST(OptionalTest, dereferenceRvalue) {
    EXPECT_EQ(*Optional<std::string>("psycho"), "psycho");
    EXPECT_EQ(Optional<std::string>("psycho")->size(), 6);
}

TEST(OptionalTest, hasValue) {
    Optional<int> v;
    EXPECT_FALSE(v.hasValue());
    v = 1;
    EXPECT_TRUE(v.hasValue());
}

TEST(OptionalTest, value) {
    {
        Optional<int> v(3);
        EXPECT_EQ(v.value(), 3);
        v.value() = 4;
        EXPECT_EQ(v.value(), 4);
    }
    {
        const Optional<int> v(5);
        EXPECT_EQ(v.value(), 5);
    }
    {
        int v = 5;
        const Optional<int&> o(v);
        EXPECT_EQ(o.value(), 5);
    }
    {
        int v = 5;
        Optional<int&> o(v);
        EXPECT_EQ(o.value(), 5);
    }
    EXPECT_EQ(Optional<int>(1).value(), 1);
}

TEST(OptionalTest, BadOptionalAccess) {
    {
        Optional<int> v;
        EXPECT_THROW(v.value(), BadOptionalAccess);
    }
    {
        const Optional<int> v;
        EXPECT_THROW(v.value(), BadOptionalAccess);
    }
    {
        Optional<std::string> v;
        EXPECT_THROW(std::move(v.value()), BadOptionalAccess);
    }
    {
        const Optional<int&> v;
        EXPECT_THROW(v.value(), BadOptionalAccess);
    }
    {
        Optional<int&> v;
        EXPECT_THROW(v.value(), BadOptionalAccess);
    }
    EXPECT_THROW(Optional<int>().value(), BadOptionalAccess);
    try {
        Optional<int>().value();
    } catch (const BadOptionalAccess& e) {
        EXPECT_NE(e.what(), nullptr);
    }
}

TEST_F(OptionalTestAware, moveFrom) {
    {
        Optional<Aware> a(getState());
        Aware b(std::move(a.value()));
        EXPECT_TRUE(getState().moveCtor);
    }
    getState() = false;
    {
        Optional<Aware> a(getState());
        Aware b(std::move(*a));
        EXPECT_TRUE(getState().moveCtor);
    }
}

TEST(OptionalTest, valueOr) {
    Optional<int> a(5);
    EXPECT_EQ(5, a.valueOr(3));
    a = NullOptional;
    EXPECT_EQ(3, a.valueOr(3));
    {
        Optional<uint64_t> ov;
        EXPECT_EQ(ov.valueOr(1), 1);
    }
    {
        Optional<std::string> ov("abc");
        EXPECT_EQ(ov.valueOr("def"), "abc");
    }
    {
        int k = 42;
        int v = 1;
        Optional<int&> ov(k);
        int& r = ov.valueOr(v);
        EXPECT_EQ(42, r);
        EXPECT_EQ(1, v);
        r = 3;
        EXPECT_EQ(3, k);
        EXPECT_EQ(1, v);
    }
    {
        const int k = 42;
        const int v = 1;
        Optional<const int&> ov(k);
        const int& r = ov.valueOr(v);
        EXPECT_EQ(42, r);
        EXPECT_EQ(1, v);
        const_cast<int&>(r) = 3;
        EXPECT_EQ(3, k);
        EXPECT_EQ(1, v);
    }
    {
        int v = 1;
        Optional<int&> ov;
        int& r = ov.valueOr(v);
        EXPECT_EQ(1, r);
        EXPECT_EQ(1, v);
        r = 3;
        EXPECT_EQ(3, v);
    }
    {
        const int v = 1;
        Optional<const int&> ov;
        const int& r = ov.valueOr(v);
        EXPECT_EQ(1, r);
        EXPECT_EQ(1, v);
        const_cast<int&>(r) = 3;
        EXPECT_EQ(3, v);
    }
    {
        const B v;
        Optional<const A&> ov;
        const A& r = ov.valueOr(v);
        (void)r;
    }
    {
        B v;
        Optional<A&> ov;
        A& r = ov.valueOr(v);
        (void)r;
    }
}

TEST(OptionalTest, equality) {
    Optional<int> oN(NullOptional);
    Optional<int> o0(0);
    Optional<int> o1(1);

    EXPECT_FALSE(o0 == o1);
    EXPECT_TRUE(o0 != o1);
    EXPECT_TRUE(o0 < o1);
    EXPECT_FALSE(o0 > o1);
    EXPECT_TRUE(o0 <= o1);
    EXPECT_FALSE(o0 >= o1);

    EXPECT_FALSE(o1 == 0);
    EXPECT_FALSE(0 == o1);
    EXPECT_TRUE(o1 != 0);
    EXPECT_TRUE(0 != o1);

    EXPECT_TRUE(oN < 0);
    EXPECT_TRUE(oN < 1);
    EXPECT_FALSE(o0 < 0);
    EXPECT_TRUE(o0 < 1);
    EXPECT_FALSE(o1 < 0);
    EXPECT_FALSE(o1 < 1);

    EXPECT_FALSE(oN >= 0);
    EXPECT_FALSE(oN >= 1);
    EXPECT_TRUE(o0 >= 0);
    EXPECT_FALSE(o0 >= 1);
    EXPECT_TRUE(o1 >= 0);
    EXPECT_TRUE(o1 >= 1);

    EXPECT_FALSE(oN > 0);
    EXPECT_FALSE(oN > 1);
    EXPECT_FALSE(o0 > 0);
    EXPECT_FALSE(o0 > 1);
    EXPECT_TRUE(o1 > 0);
    EXPECT_FALSE(o1 > 1);

    EXPECT_TRUE(oN <= 0);
    EXPECT_TRUE(oN <= 1);
    EXPECT_TRUE(o0 <= 0);
    EXPECT_TRUE(o0 <= 1);
    EXPECT_FALSE(o1 <= 0);
    EXPECT_TRUE(o1 <= 1);

    EXPECT_TRUE(0 < o1);
    EXPECT_TRUE(0 > oN);
    EXPECT_TRUE(1 > oN);
    EXPECT_FALSE(0 > o0);
    EXPECT_TRUE(1 > o0);
    EXPECT_FALSE(0 > o1);
    EXPECT_FALSE(1 > o1);

    EXPECT_TRUE(0 >= oN);
    EXPECT_FALSE(0 <= oN);
    EXPECT_FALSE(1 <= oN);
    EXPECT_TRUE(0 <= o0);
    EXPECT_FALSE(1 <= o0);
    EXPECT_TRUE(0 <= o1);
    EXPECT_TRUE(1 <= o1);

    EXPECT_FALSE(NullOptional == o1);
    EXPECT_TRUE(o1 != NullOptional);
    EXPECT_TRUE(NullOptional != o1);
    EXPECT_FALSE(o1 < NullOptional);
    EXPECT_TRUE(NullOptional < o1);
    EXPECT_FALSE(o1 <= NullOptional);
    EXPECT_TRUE(NullOptional <= o1);
    EXPECT_TRUE(o1 > NullOptional);
    EXPECT_FALSE(NullOptional > o1);
    EXPECT_TRUE(o1 >= NullOptional);
    EXPECT_FALSE(NullOptional >= o1);
}

TEST(OptionalTest, references) {
    {
        int v = 1;
        Optional<int&> ov;
        ov = v;
        EXPECT_EQ(1, *ov);
        v = 3;
        EXPECT_EQ(3, *ov);
    }
    {
        int v = 1;
        Optional<int&> ov(v);
        EXPECT_EQ(1, *ov);
        v = 3;
        EXPECT_EQ(3, *ov);
    }
}

TEST(OptionalTest, implicitConstructionFromReference) {
    bool fromA_toConstARef = IsImplicitlyConstructibleFrom<Optional<const A&>, A>::value;
    EXPECT_TRUE(fromA_toConstARef);

    bool fromARef_toConstARef = IsImplicitlyConstructibleFrom<Optional<const A&>, A&>::value;
    EXPECT_TRUE(fromARef_toConstARef);

    bool fromConstA_toConstARef = IsImplicitlyConstructibleFrom<Optional<const A&>, const A>::value;
    EXPECT_TRUE(fromConstA_toConstARef);

    bool fromConstARef_toConstARef = IsImplicitlyConstructibleFrom<Optional<const A&>, const A&>::value;
    EXPECT_TRUE(fromConstARef_toConstARef);

    bool fromA_toARef = IsImplicitlyConstructibleFrom<Optional<A&>, A>::value;
    EXPECT_FALSE(fromA_toARef);

    bool fromConstA_toARef = IsImplicitlyConstructibleFrom<Optional<A&>, const A>::value;
    EXPECT_FALSE(fromConstA_toARef);

    bool fromConstARef_toARef = IsImplicitlyConstructibleFrom<Optional<A&>, const A&>::value;
    EXPECT_FALSE(fromConstARef_toARef);

    bool fromB_toB = IsImplicitlyConstructibleFrom<Optional<const B&>, B&>::value;
    EXPECT_TRUE(fromB_toB);

    bool fromB_toA = IsImplicitlyConstructibleFrom<Optional<const A&>, B&>::value;
    EXPECT_TRUE(fromB_toA);

    bool fromA_toB = IsImplicitlyConstructibleFrom<Optional<const B&>, A&>::value;
    EXPECT_FALSE(fromA_toB);
}

TEST(OptionalTest, hash) {
    Optional<int> v(3);
    EXPECT_EQ(std::hash<Optional<int>>{}(v), std::hash<int>{}(3));
    EXPECT_EQ(std::hash<Optional<int>>{}(Optional<int>(NullOptional)), std::size_t(-23));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    return RUN_ALL_TESTS();
}
