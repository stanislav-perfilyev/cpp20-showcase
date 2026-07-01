#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 07_atomic_model/atomic_model.h  —  C++ Memory Model showcase
//
// Demonstrates with RUNNING CODE (not just comments):
//  1. Acquire/Release — producer/consumer message passing
//  2. Sequential Consistency — global ordering guarantee
//  3. Relaxed  — counter increment (no ordering needed)
//  4. Spinlock  — using atomic_flag + acquire/release
//  5. Double-Checked Locking — safe singleton with atomic
//  6. ABA problem illustration (and why SPSC avoids it)
// ─────────────────────────────────────────────────────────────────────────────
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <utility>

// ── 1. Acquire/Release — message passing ─────────────────────────────────────
//
// Without ordering: compiler/CPU may reorder stores, consumer sees data=0 even
// after flag=true. With release/acquire: the store to `data` HAPPENS BEFORE
// the store to `flag` (from producer's PoV), and the load of `flag` HAPPENS
// BEFORE the load of `data` (from consumer's PoV).

struct MessageChannel {
    std::atomic<bool> flag{false};
    int data{0};  // protected by flag's release/acquire

    // Producer: write data, then signal
    void produce(int value) noexcept {
        data = value;
        flag.store(true, std::memory_order_release);  // "publish"
    }

    // Consumer: spin until flag, then read data
    [[nodiscard]] int consume() noexcept {
        while (!flag.load(std::memory_order_acquire))  // "subscribe"
            std::this_thread::yield();
        return data;  // guaranteed to see producer's value
    }
};

// ── 2. Relaxed counter ────────────────────────────────────────────────────────
//
// When you only need atomicity (no ordering), relaxed is cheapest.
// Use case: statistics counters, hit counts — no one cares about order.

struct RelaxedCounter {
    std::atomic<std::int64_t> value{0};

    void increment() noexcept {
        value.fetch_add(1, std::memory_order_relaxed);
    }
    [[nodiscard]] std::int64_t get() const noexcept {
        // relaxed: may read a slightly stale value from another thread's PoV,
        // but the count itself is always atomically correct.
        return value.load(std::memory_order_relaxed);
    }
};

// ── 3. Spinlock (atomic_flag — guaranteed lock-free) ─────────────────────────
//
// atomic_flag is the ONLY type guaranteed lock-free by the standard.
// test_and_set = acquire; clear = release.

class Spinlock {
public:
    void lock() noexcept {
        while (m_flag.test_and_set(std::memory_order_acquire))
            std::this_thread::yield();  // spin (yield to avoid burning CPU)
    }
    void unlock() noexcept {
        m_flag.clear(std::memory_order_release);
    }

    // RAII guard
    struct Guard {
        explicit Guard(Spinlock& sl) : m_sl(sl) { m_sl.lock(); }
        ~Guard() { m_sl.unlock(); }
        Guard(const Guard&)            = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        Spinlock& m_sl;
    };

    [[nodiscard]] Guard lock_guard() noexcept { return Guard{*this}; }

private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

// ── 4. Double-Checked Locking — safe singleton ────────────────────────────────
//
// Classic (broken) DCL: two plain reads of `instance` — compiler may reorder
// constructor with pointer store. Fix: atomic store/load with release/acquire.

template<typename T>
class LazySingleton {
public:
    template<typename... Args>
    [[nodiscard]] static T& instance(Args&&... args) {
        // Fast path: acquire load (sees any release from init thread)
        T* ptr = s_instance.load(std::memory_order_acquire);
        if (ptr == nullptr) {
            std::lock_guard lock{s_mutex};
            ptr = s_instance.load(std::memory_order_relaxed);
            if (ptr == nullptr) {
                ptr = new T(std::forward<Args>(args)...);
                s_instance.store(ptr, std::memory_order_release);
            }
        }
        return *ptr;
    }

private:
    static std::atomic<T*>  s_instance;
    static std::mutex        s_mutex;
};

template<typename T> std::atomic<T*>  LazySingleton<T>::s_instance{nullptr};
template<typename T> std::mutex        LazySingleton<T>::s_mutex;

// ── 5. SeqCst: total order guarantee ─────────────────────────────────────────
//
// With seq_cst all threads agree on a single global order of atomic ops.
// More expensive (full fence on x86), but simplest reasoning model.
// Dekker's critical section: shows seq_cst prevents both threads entering at once.

struct SeqCstFlag {
    std::atomic<bool> wants_enter{false};

    // Returns false if another thread is also trying → retry
    [[nodiscard]] bool try_enter(std::atomic<bool>& other_wants) noexcept {
        wants_enter.store(true, std::memory_order_seq_cst);
        // With seq_cst: if other thread also set its flag, BOTH see each other.
        // Without seq_cst: could miss each other → both enter critical section.
        if (other_wants.load(std::memory_order_seq_cst)) {
            wants_enter.store(false, std::memory_order_seq_cst);
            return false;
        }
        return true;
    }
    void exit() noexcept { wants_enter.store(false, std::memory_order_seq_cst); }
};
