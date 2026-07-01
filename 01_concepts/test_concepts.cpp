// ─────────────────────────────────────────────────────────────────────────────
// 01_concepts/test_concepts.cpp  —  Google Test for C++20 Concepts
// ─────────────────────────────────────────────────────────────────────────────
#include "concepts.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
#include <list>

// ── Concept satisfaction checks (static_assert) ───────────────────────────────
static_assert(Numeric<int>);
static_assert(Numeric<double>);
static_assert(!Numeric<std::string>);

static_assert(Printable<int>);
static_assert(Printable<std::string>);

static_assert(Sortable<int>);
static_assert(Sortable<double>);
static_assert(Sortable<std::string>);

static_assert(Hashable<int>);
static_assert(Hashable<std::string>);

static_assert(Container<std::vector<int>>);
static_assert(Container<std::list<double>>);
static_assert(!Container<int>);

// ── clamp() tests ─────────────────────────────────────────────────────────────
TEST(ConceptsClamp, ClampInt_WithinRange) {
    EXPECT_EQ(clamp(5, 0, 10), 5);
}
TEST(ConceptsClamp, ClampInt_BelowMin) {
    EXPECT_EQ(clamp(-5, 0, 10), 0);
}
TEST(ConceptsClamp, ClampInt_AboveMax) {
    EXPECT_EQ(clamp(15, 0, 10), 10);
}
TEST(ConceptsClamp, ClampDouble) {
    EXPECT_DOUBLE_EQ(clamp(1.5, 0.0, 1.0), 1.0);
}

// ── sum() tests ───────────────────────────────────────────────────────────────
TEST(ConceptsSum, SumVector) {
    std::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(sum(v), 15);
}
TEST(ConceptsSum, SumEmptyVector) {
    std::vector<double> v{};
    EXPECT_DOUBLE_EQ(sum(v), 0.0);
}
TEST(ConceptsSum, SumList) {
    std::list<int> l{10, 20, 30};
    EXPECT_EQ(sum(l), 60);
}

// ── TypedStack tests ──────────────────────────────────────────────────────────
TEST(TypedStack, PushAndTop) {
    TypedStack<int> s;
    s.push(42);
    EXPECT_EQ(s.top(), 42);
    EXPECT_EQ(s.size(), 1u);
}
TEST(TypedStack, PopReducesSize) {
    TypedStack<int> s;
    s.push(1); s.push(2);
    s.pop();
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s.top(), 1);
}
TEST(TypedStack, MinMax) {
    TypedStack<int> s;
    s.push(3); s.push(1); s.push(4); s.push(1); s.push(5);
    EXPECT_EQ(s.min(), 1);
    EXPECT_EQ(s.max(), 5);
}
TEST(TypedStack, TopOnEmptyThrows) {
    TypedStack<double> s;
    EXPECT_THROW(s.top(), std::runtime_error);
}
TEST(TypedStack, PopOnEmptyThrows) {
    TypedStack<double> s;
    EXPECT_THROW(s.pop(), std::runtime_error);
}
TEST(TypedStack, StringStack) {
    TypedStack<std::string> s;
    s.push("hello"); s.push("world");
    EXPECT_EQ(s.top(), "world");
    EXPECT_EQ(s.min(), "hello");
}
TEST(TypedStack, PrintToStream) {
    TypedStack<int> s;
    s.push(1); s.push(2); s.push(3);
    std::ostringstream oss;
    s.print(oss);
    EXPECT_EQ(oss.str(), "TypedStack[1, 2, 3]\n");
}

// ── display() overload resolution ─────────────────────────────────────────────
TEST(ConceptsDisplay, NumericOverloadSelected) {
    std::ostringstream oss;
    display(42, oss);
    EXPECT_TRUE(oss.str().find("[Numeric+Printable]") != std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
