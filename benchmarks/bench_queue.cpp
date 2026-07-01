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
static void BM_SPSCQueue(benchmark::State& state) {
    constexpr std::size_t kCap = 4096;
    SPSCQueue<int, kCap> q;
    std::atomic<bool> done{false};
    long long items = state.range(0);

    std::thread producer([&] {
        for (long long i = 0; i < items; ++i)
            while (!q.push(static_cast<int>(i))) std::this_thread::yield();
        done.store(true, std::memory_order_release);
    });

    for (auto _ : state) {
        int received = 0;
        long long count = 0;
        while (count < items) {
            if (q.pop(received)) ++count;
            else std::this_thread::yield();
        }
        benchmark::DoNotOptimize(received);
    }
    producer.join();
    state.SetItemsProcessed(state.iterations() * items);
}
BENCHMARK(BM_SPSCQueue)->Arg(100'000)->UseRealTime();

// ── Mutex-guarded std::queue ──────────────────────────────────────────────────
static void BM_MutexQueue(benchmark::State& state) {
    std::queue<int> q;
    std::mutex      mtx;
    std::atomic<bool> done{false};
    long long items = state.range(0);

    std::thread producer([&] {
        for (long long i = 0; i < items; ++i) {
            std::lock_guard lock{mtx};
            q.push(static_cast<int>(i));
        }
        done.store(true, std::memory_order_release);
    });

    for (auto _ : state) {
        long long count = 0;
        while (count < items) {
            std::lock_guard lock{mtx};
            if (!q.empty()) { q.front(); q.pop(); ++count; }
        }
    }
    producer.join();
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
