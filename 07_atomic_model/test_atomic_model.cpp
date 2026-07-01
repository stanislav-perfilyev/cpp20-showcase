// ─────────────────────────────────────────────────────────────────────────────
// 07_atomic_model/test_atomic_model.cpp  —  Google Test for Memory Model demos
// ─────────────────────────────────────────────────────────────────────────────
#include "atomic_model.h"
#include <gtest/gtest.h>
#include <numeric>
#include <thread>
#include <vector>

// ── MessageChannel (acquire/release) ─────────────────────────────────────────
TEST(AtomicModel, MessageChannel_DataVisible) {
    MessageChannel ch;
    int received = -1;

    std::jthread consumer([&](std::stop_token) {
        received = ch.consume();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ch.produce(42);
    // consumer will join here (jthread destructor)
    consumer.join();

    EXPECT_EQ(received, 42);
}

// ── RelaxedCounter ────────────────────────────────────────────────────────────
TEST(AtomicModel, RelaxedCounter_Correct) {
    RelaxedCounter c;
    constexpr int N = 100'000;
    constexpr int T = 4;

    std::vector<std::jthread> threads;
    for (int i = 0; i < T; ++i)
        threads.emplace_back([&](std::stop_token) {
            for (int j = 0; j < N; ++j) c.increment();
        });
    threads.clear();  // join all

    EXPECT_EQ(c.get(), static_cast<std::int64_t>(T) * N);
}

// ── Spinlock ──────────────────────────────────────────────────────────────────
TEST(AtomicModel, Spinlock_ProtectsSharedCounter) {
    Spinlock sl;
    int counter = 0;
    constexpr int N = 50'000;
    constexpr int T = 4;

    std::vector<std::jthread> threads;
    for (int i = 0; i < T; ++i)
        threads.emplace_back([&](std::stop_token) {
            for (int j = 0; j < N; ++j) {
                auto g = sl.lock_guard();
                ++counter;  // critical section
            }
        });
    threads.clear();

    EXPECT_EQ(counter, T * N);
}

// ── LazySingleton (DCL) ───────────────────────────────────────────────────────
TEST(AtomicModel, LazySingleton_SameInstance) {
    // All threads must get the same instance
    constexpr int T = 8;
    std::vector<int*> ptrs(T);
    std::vector<std::jthread> threads;
    for (int i = 0; i < T; ++i)
        threads.emplace_back([&, i](std::stop_token) {
            ptrs[static_cast<size_t>(i)] = &LazySingleton<int>::instance(0);
        });
    threads.clear();

    // All pointers must be equal
    for (int i = 1; i < T; ++i)
        EXPECT_EQ(ptrs[0], ptrs[static_cast<size_t>(i)]);
}

// ── SeqCst mutual exclusion ───────────────────────────────────────────────────
TEST(AtomicModel, SeqCst_NoDualEntry) {
    SeqCstFlag f0, f1;
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    constexpr int N = 10'000;

    auto body = [&](SeqCstFlag& mine, SeqCstFlag& other) {
        for (int i = 0; i < N; ++i) {
            // Retry until we can enter
            while (!mine.try_enter(other.wants_enter))
                std::this_thread::yield();
            // Inside "critical section"
            int c = concurrent_count.fetch_add(1, std::memory_order_seq_cst) + 1;
            int cur_max = max_concurrent.load(std::memory_order_relaxed);
            while (c > cur_max &&
                   !max_concurrent.compare_exchange_weak(cur_max, c,
                       std::memory_order_relaxed))
                {}
            concurrent_count.fetch_sub(1, std::memory_order_seq_cst);
            mine.exit();
        }
    };

    std::jthread t0([&](std::stop_token) { body(f0, f1); });
    std::jthread t1([&](std::stop_token) { body(f1, f0); });
    t0.join(); t1.join();

    // At most 1 thread should be in the critical section at a time
    EXPECT_LE(max_concurrent.load(), 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
