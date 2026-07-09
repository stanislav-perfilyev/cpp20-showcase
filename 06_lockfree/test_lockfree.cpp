// ─────────────────────────────────────────────────────────────────────────────
// 06_lockfree/test_lockfree.cpp  —  Google Test for SPSCQueue
// Includes single-threaded correctness + multi-threaded stress test
// ─────────────────────────────────────────────────────────────────────────────
#include "spsc_queue.h"
#include <gtest/gtest.h>
#include <atomic>
#include <numeric>
#include <string>
#include <chrono>
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
    ASSERT_TRUE(q.push(7));
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
    ASSERT_TRUE(q.push(1)); ASSERT_TRUE(q.push(2));
    EXPECT_EQ(q.size_approx(), 2u);
    int v; ASSERT_TRUE(q.pop(v));
    EXPECT_EQ(q.size_approx(), 1u);
}


// ── Multi-threaded throughput & latency ───────────────────────────────────────

TEST(SPSCQueue, MT_Throughput_Measurement) {
    // Measures real concurrent throughput: producer + consumer in separate jthreads
    constexpr int N = 2'000'000;
    SPSCQueue<int, 65536> q;
    std::atomic<bool> ready{false};

    auto t0 = std::chrono::steady_clock::now();

    std::jthread producer([&](std::stop_token) {
        ready.wait(false, std::memory_order_acquire);
        for (int i = 0; i < N; ++i)
            while (!q.push(i)) std::this_thread::yield();
    });

    std::jthread consumer([&](std::stop_token) {
        ready.wait(false, std::memory_order_acquire);
        int v = 0, received = 0;
        while (received < N) {
            if (q.pop(v)) ++received;
            else std::this_thread::yield();
        }
    });

    ready.store(true, std::memory_order_release);
    ready.notify_all();

    producer.join();
    consumer.join();

    const double elapsed_s = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    const double mops = N / elapsed_s / 1e6;
    std::printf("[SPSCQueue Throughput] %.2f M ops/sec (N=%d, %.3f s)\n",
                mops, N, elapsed_s);

    EXPECT_GT(mops, 0.1); // >100k ops/sec — conservative for any CI machine
}

TEST(SPSCQueue, MT_FIFO_Ordering_Verified) {
    // Verifies strict FIFO ordering under concurrent push/pop
    constexpr int N = 100'000;
    SPSCQueue<int, 4096> q;
    std::vector<int> received;
    received.reserve(N);

    std::jthread producer([&](std::stop_token) {
        for (int i = 0; i < N; ++i)
            while (!q.push(i)) std::this_thread::yield();
    });

    std::jthread consumer([&](std::stop_token) {
        int v = 0;
        while (static_cast<int>(received.size()) < N) {
            if (q.pop(v)) received.push_back(v);
            else std::this_thread::yield();
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(static_cast<int>(received.size()), N);
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(received[i], i) << "FIFO violated at index " << i;
}

TEST(SPSCQueue, MT_Latency_Mean_Under100us) {
    // Measures mean round-trip time: push timestamp → pop timestamp
    constexpr int N = 10'000;
    SPSCQueue<int64_t, 256> q; // pass timestamps
    std::atomic<uint64_t>   total_ns{0};

    std::jthread producer([&](std::stop_token) {
        for (int i = 0; i < N; ++i) {
            const int64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            while (!q.push(ts)) std::this_thread::yield();
            std::this_thread::yield(); // simulate inter-arrival gap
        }
    });

    std::jthread consumer([&](std::stop_token) {
        int64_t ts = 0;
        for (int received = 0; received < N; ) {
            if (!q.pop(ts)) { std::this_thread::yield(); continue; }
            const int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            total_ns.fetch_add(static_cast<uint64_t>(now - ts), std::memory_order_relaxed);
            ++received;
        }
    });

    producer.join();
    consumer.join();

    const double mean_us = static_cast<double>(total_ns.load()) / N / 1000.0;
    std::printf("[SPSCQueue Latency] Mean push→pop: %.2f µs\n", mean_us);
    EXPECT_LT(mean_us, 100.0); // <100µs mean on any reasonable machine
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
