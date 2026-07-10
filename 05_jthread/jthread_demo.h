#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 05_jthread/jthread_demo.h  —  C++20 jthread + stop_token + latch + barrier
//
// Demonstrates:
//  • std::jthread: auto-join RAII thread
//  • std::stop_token / stop_source / stop_callback
//  • std::latch: one-time countdown (e.g. "wait for N workers to start")
//  • std::barrier: reusable phase synchronisation
//  • std::counting_semaphore: rate-limiting producer/consumer
// ─────────────────────────────────────────────────────────────────────────────
#include <atomic>
#include <barrier>
#include <chrono>
#include <functional>
#include <latch>
#include <mutex>
#include <semaphore>
#include <stop_token>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ── StoppableWorker ───────────────────────────────────────────────────────────

/// A cancellable background worker.
/// Polls stop_token periodically; stop() is called automatically on destruction.
class StoppableWorker {
public:
    explicit StoppableWorker(std::function<void(std::stop_token)> work)
        : m_thread(std::move(work))
    {}

    // jthread destructor calls request_stop() + join() — RAII, no leaks.
    ~StoppableWorker() = default;

    StoppableWorker(const StoppableWorker&)            = delete;
    StoppableWorker& operator=(const StoppableWorker&) = delete;
    StoppableWorker(StoppableWorker&&)                 = default;
    StoppableWorker& operator=(StoppableWorker&&)      = default;

    void stop() noexcept { m_thread.request_stop(); }
    [[nodiscard]] bool stop_requested() const noexcept {
        return m_thread.get_stop_token().stop_requested();
    }

private:
    std::jthread m_thread;
};

// ── LatchBarrier demo utilities ───────────────────────────────────────────────

/// Starts N workers, waits until all have initialised (latch), then returns.
/// Each worker increments a shared counter once per "phase".
/// Returns total sum computed by all workers.
[[nodiscard]] inline long long
parallel_sum_with_latch(int n_workers, int iterations) {
    std::atomic<long long> total{0};
    std::latch             ready{n_workers};  // count-down to 0 when all workers ready
    std::vector<std::jthread> workers;
    workers.reserve(static_cast<size_t>(n_workers));

    for (int i = 0; i < n_workers; ++i) {
        workers.emplace_back([&](std::stop_token) {
            // Signal "ready" — coordinator waits for this
            ready.count_down();
            // Do actual work
            for (int j = 0; j < iterations; ++j)
                total.fetch_add(1, std::memory_order_relaxed);
        });
    }
    ready.wait();       // blocks until all workers called count_down()
    // jthreads join on destruction of 'workers'
    workers.clear();
    return total.load();
}

/// Barrier demo: N threads run PHASES phases, synchronised at each boundary.
/// Returns a vector: result[phase] = how many threads completed that phase.
[[nodiscard]] inline std::vector<int>
barrier_phases(int n_threads, int n_phases) {
    std::vector<int> phase_count(static_cast<size_t>(n_phases), 0);
    std::mutex       mtx;

    std::barrier sync{n_threads, [&]() noexcept {
        // completion function: called once per phase after all threads arrive
        // (runs on the last arriving thread — must be noexcept)
    }};

    std::vector<std::jthread> threads;
    threads.reserve(static_cast<size_t>(n_threads));

    for (int t = 0; t < n_threads; ++t) {
        threads.emplace_back([&](std::stop_token) {
            for (int p = 0; p < n_phases; ++p) {
                // ... work for phase p ...
                sync.arrive_and_wait();  // wait for everyone at end of phase
                {
                    std::lock_guard lock{mtx};
                    phase_count[static_cast<size_t>(p)]++;
                }
            }
        });
    }
    threads.clear();  // join all
    return phase_count;
}

// ── Semaphore-based rate limiter ──────────────────────────────────────────────

/// Produces 'count' items, limited to at most 'max_concurrent' in flight at once.
/// Returns total items processed.
[[nodiscard]] inline int
semaphore_rate_limit(int count, int max_concurrent) {
    std::counting_semaphore<128> sem{max_concurrent};
    std::atomic<int>             processed{0};
    std::vector<std::jthread>    workers;
    workers.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        workers.emplace_back([&](std::stop_token) {
            sem.acquire();          // blocks if max_concurrent already running
            processed.fetch_add(1, std::memory_order_relaxed);
            sem.release();
        });
    }
    workers.clear();
    return processed.load();
}
