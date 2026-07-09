// ─────────────────────────────────────────────────────────────────────────────
// 03_coroutines/test_coroutines.cpp  —  Google Test for C++20 Coroutines
// ─────────────────────────────────────────────────────────────────────────────
#include "generator.h"
#include "task.h"
#include <gtest/gtest.h>
#include <ranges>
#include <vector>

// ── Generator<T> tests ────────────────────────────────────────────────────────
TEST(Generator, Fibonacci_First10) {
    std::vector<long long> expected{0,1,1,2,3,5,8,13,21,34};
    std::vector<long long> got;
    for (auto v : fibonacci() | std::views::take(10))
        got.push_back(v);
    EXPECT_EQ(got, expected);
}
TEST(Generator, IRange_Basic) {
    std::vector<long long> got;
    for (auto v : irange(0, 5))
        got.push_back(v);
    EXPECT_EQ(got, (std::vector<long long>{0,1,2,3,4}));
}
TEST(Generator, IRange_Step2) {
    std::vector<long long> got;
    for (auto v : irange(0, 10, 2))
        got.push_back(v);
    EXPECT_EQ(got, (std::vector<long long>{0,2,4,6,8}));
}
TEST(Generator, IRange_Descending) {
    std::vector<long long> got;
    for (auto v : irange(5, 0, -1))
        got.push_back(v);
    EXPECT_EQ(got, (std::vector<long long>{5,4,3,2,1}));
}
TEST(Generator, IRange_Empty) {
    std::vector<long long> got;
    for (auto v : irange(5, 5))
        got.push_back(v);
    EXPECT_TRUE(got.empty());
}
TEST(Generator, IRange_ZeroStep_Throws) {
    EXPECT_THROW(irange(0, 5, 0), std::invalid_argument);
}
TEST(Generator, FromRange) {
    std::vector<int> src{10, 20, 30};
    std::vector<int> got;
    for (auto v : from_range(src) | std::views::take(2))
        got.push_back(v);
    EXPECT_EQ(got, (std::vector<int>{10, 20}));
}
TEST(Generator, MoveSemanticsOk) {
    auto g1 = fibonacci();
    auto g2 = std::move(g1);  // Move should not crash
    std::vector<long long> got;
    for (auto v : g2 | std::views::take(3))
        got.push_back(v);
    EXPECT_EQ(got, (std::vector<long long>{0,1,1}));
}

// ── Task<T> tests ─────────────────────────────────────────────────────────────
namespace {
Task<int> async_add(int a, int b) {
    co_return a + b;
}

Task<int> async_chain(int x) {
    int r1 = co_await async_add(x, 10);
    int r2 = co_await async_add(r1, 5);
    co_return r2;
}

Task<int> async_throws() {
    throw std::runtime_error("async error");
    // cppcheck-suppress unreachableCode
    co_return 0;  // required: coroutine must have co_return path
}
} // namespace

TEST(Task, SimpleReturn) {
    EXPECT_EQ(async_add(3, 4).sync_wait(), 7);
}
TEST(Task, ChainedTasks) {
    // async_chain(10) → add(10,10)=20 → add(20,5)=25
    EXPECT_EQ(async_chain(10).sync_wait(), 25);
}
TEST(Task, ExceptionPropagates) {
    // [[nodiscard]] on sync_wait(): use maybe_unused to suppress -Wunused-result
    EXPECT_THROW(
        { [[maybe_unused]] auto r = async_throws().sync_wait(); },
        std::runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
