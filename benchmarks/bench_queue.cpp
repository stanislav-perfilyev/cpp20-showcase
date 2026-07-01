// ─────────────────────────────────────────────────────────────────────────────
// benchmarks/bench_queue.cpp  —  Google Benchmark: lock-free vs mutex queue
//
// Measures SPSC throughput for:
//  1. SPSCQueue (lock-free, acquire/release)
//  2. std::queue + std::mutex (baseline)
//  3. std::atomic<int> increment (no queue — pure atomic cost)
// ─────────────────────────────────────────────────────────────────────────────
#include "../06_lockfree/spsc_queue.h"
#include <benchmark/benchmark.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

// ── Lock-free SPSC ────────────────────────────────────────────────────────────
//
// NOTE: producer must be created INSIDE the iteration loop.
// If created outside, it pushes items only once — subsequent iterations
// spin forever waiting for items that will never come (deadlock).
static void BM_SPSCQueue(benchmark::State& state) {
    constexpr std::size_t kCap = 4096;
    const long long items = state.range(0);

    for (auto _ : state) {
        SPSCQueue<int, kCap> q;
        int received = 0;

        std::jthread producer([&](std::stop_token) {
            for (long long i = 0; i < items; ++i)
                while (!q.push(static_cast<int>(i))) std::this_thread::yield();
        });

        for (long long count = 0; count < items;) {
            if (q.pop(received)) ++count;
            else std::this_thread::yield();
        }
        benchmark::DoNotOptimize(received);
        // producer joins here on scope exit (jthread RAII)
    }
    state.SetItemsProcessed(state.iterations() * items);
}
BENCHMARK(BM_SPSCQueue)->Arg(100'000)->UseRealTime();

// ── Mutex-guarded std::queue ──────────────────────────────────────────────────
static void BM_MutexQueue(benchmark::State& state) {
    const long long items = state.range(0);

    for (auto _ : state) {
        std::queue<int> q;
        std::mutex      mtx;

        std::jthread producer([&](std::stop_token) {
            for (long long i = 0; i < items; ++i) {
                std::lock_guard lock{mtx};
                q.push(static_cast<int>(i));
            }
        });

        for (long long count = 0; count < items;) {
            std::lock_guard lock{mtx};
            if (!q.empty()) { benchmark::DoNotOptimize(q.front()); q.pop(); ++count; }
        }
        // producer joins on scope exit
    }
    state.SetItemsProcessed(state.iterations() * items);
}
BENCHMARK(BM_MutexQueue)->Arg(100'000)->UseRealTime();

// ── Pure atomic increment (lower bound) ──────────────────────────────────────
static void BM_AtomicIncrement(benchmark::State& state) {
    std::atomic<long long> counter{0};
    for (auto _ : state)
        counter.fetch_add(1, std::memory_order_relaxed);
    benchmark::DoNotOptimize(counter.load());
}
BENCHMARK(BM_AtomicIncrement);

// ── Spinlock vs mutex (small critical section) ────────────────────────────────
#include "../07_atomic_model/atomic_model.h"

static void BM_Spinlock(benchmark::State& state) {
    Spinlock sl;
    int counter = 0;
    for (auto _ : state) {
        auto g = sl.lock_guard();
        benchmark::DoNotOptimize(++counter);
    }
}
BENCHMARK(BM_Spinlock)->Threads(1)->Threads(4);

static void BM_StdMutex(benchmark::State& state) {
    std::mutex mtx;
    int counter = 0;
    for (auto _ : state) {
        std::lock_guard lock{mtx};
        benchmark::DoNotOptimize(++counter);
    }
}
BENCHMARK(BM_StdMutex)->Threads(1)->Threads(4);

BENCHMARK_MAIN();
