#include "lib-optional/optional.hpp"

#include <gmock/gmock.h>
#include <sstream>
#include <string>

using namespace libOptional;

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
}

TEST(OptionalTest, convertingCopyCtor) {
    Optional<std::string> a("This is a test string");
    Optional<std::ostringstream> b(a);
    EXPECT_TRUE(bool(a));
    EXPECT_TRUE(bool(b));
    EXPECT_EQ(*a, b->str());
}

TEST(OptionalTest, convertingMoveCtor) {
    Optional<std::string> a("This is a test string");
    Optional<std::ostringstream> b(std::move(a));
    EXPECT_TRUE(bool(a));
    EXPECT_TRUE(bool(b));
    EXPECT_EQ(*a, b->str());
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

TEST_F(OptionalTestAware, reset) {
    Optional<Aware> a(getState());
    EXPECT_TRUE(bool(a));
    a.reset();
    EXPECT_FALSE(bool(a));
    EXPECT_TRUE(getState().dtor);
}

TEST_F(OptionalTestAware, swap) {
    Optional<std::string> a("A");
    Optional<std::string> b("B");
    std::swap(a, b);
    EXPECT_TRUE(bool(a));
    EXPECT_TRUE(bool(b));

    EXPECT_EQ(*a, "B");
    EXPECT_EQ(*b, "A");
}

TEST(OptionalTest, hasValue) {
    Optional<int> v;
    EXPECT_FALSE(v.hasValue());
    v = 1;
    EXPECT_TRUE(v.hasValue());
}

TEST(OptionalTest, value) {
    Optional<int> v(3);
    EXPECT_EQ(v.value(), 3);
}

TEST(OptionalTest, BadOptionalAccess) {
    Optional<int> v;
    EXPECT_THROW(v.value(), BadOptionalAccess);
}

TEST_F(OptionalTestAware, moveFrom) {
    Optional<Aware> a(getState());
    Optional<Aware> b(std::move(a.value()));
    EXPECT_TRUE(getState().moveCtor);
}

TEST(OptionalTest, valueOr) {
    Optional<int> a(5);
    EXPECT_EQ(5, a.valueOr(3));
    a = NullOptional;
    EXPECT_EQ(3, a.valueOr(3));
}

TEST(OptionalTest, equality) {
    Optional<int> oN(NullOptional);
    Optional<int> o0(0);
    Optional<int> o1(1);

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

    EXPECT_TRUE(0 > oN);
    EXPECT_TRUE(1 > oN);
    EXPECT_FALSE(0 > o0);
    EXPECT_TRUE(1 > o0);
    EXPECT_FALSE(0 > o1);
    EXPECT_FALSE(1 > o1);

    EXPECT_FALSE(0 <= oN);
    EXPECT_FALSE(1 <= oN);
    EXPECT_TRUE(0 <= o0);
    EXPECT_FALSE(1 <= o0);
    EXPECT_TRUE(0 <= o1);
    EXPECT_TRUE(1 <= o1);
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
