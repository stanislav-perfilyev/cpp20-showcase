// ─────────────────────────────────────────────────────────────────────────────
// 05_jthread/test_jthread.cpp  —  Google Test for jthread/latch/barrier
// ─────────────────────────────────────────────────────────────────────────────
#include "jthread_demo.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

// ── StoppableWorker ───────────────────────────────────────────────────────────
TEST(JThread, StoppableWorker_StopsCleanly) {
    std::atomic<int> ticks{0};
    {
        StoppableWorker w([&](std::stop_token st) {
            while (!st.stop_requested()) {
                ticks.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(1ms);
            }
        });
        std::this_thread::sleep_for(20ms);
        w.stop();
    } // destructor joins
    EXPECT_GT(ticks.load(), 0);
}

TEST(JThread, StoppableWorker_StopCallback) {
    std::atomic<bool> callback_called{false};
    {
        StoppableWorker w([&](std::stop_token st) {
            // Register a callback that fires when stop is requested
            std::stop_callback cb{st, [&]{ callback_called.store(true); }};
            while (!st.stop_requested())
                std::this_thread::sleep_for(1ms);
        });
        std::this_thread::sleep_for(5ms);
    } // destructor: request_stop() → callback fires, then join
    EXPECT_TRUE(callback_called.load());
}

// ── Latch ─────────────────────────────────────────────────────────────────────
TEST(Latch, ParallelSumCorrect) {
    constexpr int workers    = 4;
    constexpr int iterations = 1000;
    long long result = parallel_sum_with_latch(workers, iterations);
    EXPECT_EQ(result, static_cast<long long>(workers) * iterations);
}

TEST(Latch, SingleWorker) {
    EXPECT_EQ(parallel_sum_with_latch(1, 500), 500LL);
}

// ── Barrier ───────────────────────────────────────────────────────────────────
TEST(Barrier, AllThreadsCompleteAllPhases) {
    constexpr int threads = 4;
    constexpr int phases  = 3;
    auto counts = barrier_phases(threads, phases);
    ASSERT_EQ(static_cast<int>(counts.size()), phases);
    // Every thread completes every phase: count == threads for each phase
    for (int p = 0; p < phases; ++p)
        EXPECT_EQ(counts[static_cast<size_t>(p)], threads)
            << "Phase " << p << " incomplete";
}

// ── Semaphore ─────────────────────────────────────────────────────────────────
TEST(Semaphore, AllItemsProcessed) {
    EXPECT_EQ(semaphore_rate_limit(100, 4), 100);
}
TEST(Semaphore, SingleConcurrent) {
    EXPECT_EQ(semaphore_rate_limit(20, 1), 20);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
