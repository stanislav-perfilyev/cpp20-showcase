// ─────────────────────────────────────────────────────────────────────────────
// 06_lockfree/test_lockfree.cpp  —  Google Test for SPSCQueue
// Includes single-threaded correctness + multi-threaded stress test
// ─────────────────────────────────────────────────────────────────────────────
#include "spsc_queue.h"
#include <gtest/gtest.h>
#include <atomic>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// ── Static assertions ─────────────────────────────────────────────────────────
static_assert(SPSCQueue<int, 4>::capacity() == 4);

// ── Single-threaded correctness ───────────────────────────────────────────────
TEST(SPSCQueue, PushPop_Basic) {
    SPSCQueue<int, 4> q;
    EXPECT_TRUE(q.push(42));
    int v = 0;
    EXPECT_TRUE(q.pop(v));
    EXPECT_EQ(v, 42);
}

TEST(SPSCQueue, Empty_PopReturnsFalse) {
    SPSCQueue<int, 4> q;
    int v = 0;
    EXPECT_FALSE(q.pop(v));
}

TEST(SPSCQueue, Full_PushReturnsFalse) {
    SPSCQueue<int, 4> q;
    // Capacity=4 → max 3 items (one slot reserved for full detection)
    int v = 0;
    int pushed = 0;
    while (q.push(pushed)) ++pushed;
    EXPECT_GT(pushed, 0);
    EXPECT_FALSE(q.push(99));
}

TEST(SPSCQueue, FIFO_Order) {
    SPSCQueue<int, 8> q;
    for (int i = 0; i < 5; ++i) ASSERT_TRUE(q.push(i));
    for (int i = 0; i < 5; ++i) {
        int v = -1;
        ASSERT_TRUE(q.pop(v));
        EXPECT_EQ(v, i);
    }
}

TEST(SPSCQueue, Optional_Pop) {
    SPSCQueue<int, 4> q;
    EXPECT_FALSE(q.pop().has_value());
    q.push(7);
    auto opt = q.pop();
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(*opt, 7);
}

TEST(SPSCQueue, WrapAround) {
    SPSCQueue<int, 4> q;
    // Push 2, pop 2, push 2 — exercises wrap-around
    ASSERT_TRUE(q.push(1)); ASSERT_TRUE(q.push(2));
    int v;
    ASSERT_TRUE(q.pop(v)); EXPECT_EQ(v, 1);
    ASSERT_TRUE(q.pop(v)); EXPECT_EQ(v, 2);
    ASSERT_TRUE(q.push(3)); ASSERT_TRUE(q.push(4));
    ASSERT_TRUE(q.pop(v)); EXPECT_EQ(v, 3);
    ASSERT_TRUE(q.pop(v)); EXPECT_EQ(v, 4);
}

TEST(SPSCQueue, MoveOnlyType) {
    SPSCQueue<std::string, 4> q;
    std::string s = "hello";
    EXPECT_TRUE(q.push(std::move(s)));
    std::string out;
    EXPECT_TRUE(q.pop(out));
    EXPECT_EQ(out, "hello");
}

// ── Multi-threaded stress test ────────────────────────────────────────────────
TEST(SPSCQueue, StressTest_1M_Items) {
    constexpr int N = 1'000'000;
    SPSCQueue<int, 1024> q;
    std::atomic<long long> sum_produced{0}, sum_consumed{0};

    std::jthread producer([&](std::stop_token) {
        for (int i = 0; i < N; ++i) {
            while (!q.push(i)) std::this_thread::yield();
            sum_produced.fetch_add(i, std::memory_order_relaxed);
        }
    });

    std::jthread consumer([&](std::stop_token) {
        int received = 0, v = 0;
        while (received < N) {
            if (q.pop(v)) {
                sum_consumed.fetch_add(v, std::memory_order_relaxed);
                ++received;
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Both jthreads join on destruction

    // Every integer from 0..N-1 must have been transferred exactly once
    EXPECT_EQ(sum_produced.load(), sum_consumed.load());
    long long expected = static_cast<long long>(N) * (N - 1) / 2;
    EXPECT_EQ(sum_consumed.load(), expected);
}

TEST(SPSCQueue, SizeApprox_SingleThread) {
    SPSCQueue<int, 8> q;
    EXPECT_EQ(q.size_approx(), 0u);
    q.push(1); q.push(2);
    EXPECT_EQ(q.size_approx(), 2u);
    int v; q.pop(v);
    EXPECT_EQ(q.size_approx(), 1u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
