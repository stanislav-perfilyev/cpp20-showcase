#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 06_lockfree/spsc_queue.h  —  Lock-free SPSC Queue
//
// Single-Producer / Single-Consumer queue using std::atomic.
//
// Design choices (every choice has a reason):
//  • Capacity = power-of-2  →  bitmask instead of modulo (no division in hot path)
//  • alignas(64) on head/tail  →  prevents false sharing (cache-line isolation)
//  • memory_order_release on store, acquire on load  →  correct happens-before,
//    cheaper than seq_cst (no full fence on x86, saves ~1 cycle)
//  • [[nodiscard]] on push/pop  →  caller must handle "queue full/empty"
//  • noexcept on hot path  →  no exception overhead in tight loops
//
// Thread safety:
//  • push() — must be called from exactly ONE thread (producer)
//  • pop()  — must be called from exactly ONE thread (consumer)
//  • size_approx() — safe to call from any thread (approximate)
// ─────────────────────────────────────────────────────────────────────────────
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

template<typename T, std::size_t Capacity>
    requires (std::is_nothrow_move_constructible_v<T>)
class SPSCQueue {
    static_assert(Capacity >= 2,          "Capacity must be at least 2");
    static_assert(std::has_single_bit(Capacity),
                  "Capacity must be a power of 2 (enables bitmask instead of modulo)");

    static constexpr std::size_t kMask = Capacity - 1;  // x & kMask == x % Capacity

public:
    // ── Producer API ─────────────────────────────────────────────────────────

    /// Enqueue by copy. Returns false if queue is full (non-blocking).
    [[nodiscard]] bool push(const T& item) noexcept
        requires std::is_nothrow_copy_constructible_v<T>
    {
        return push_impl(item);
    }

    /// Enqueue by move. Returns false if queue is full (non-blocking).
    [[nodiscard]] bool push(T&& item) noexcept {
        return push_impl(std::move(item));
    }

    // ── Consumer API ─────────────────────────────────────────────────────────

    /// Dequeue into out. Returns false if queue is empty (non-blocking).
    [[nodiscard]] bool pop(T& out) noexcept {
        const std::size_t tail = m_tail.load(std::memory_order_relaxed);

        // Acquire: see all writes the producer did before its release-store.
        if (tail == m_head.load(std::memory_order_acquire))
            return false;  // empty

        out = std::move(m_buffer[tail & kMask]);

        // Release: make the slot visible to producer as "free".
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }

    /// Dequeue and return wrapped in std::optional (convenience).
    [[nodiscard]] std::optional<T> pop() noexcept {
        T value{};
        if (!pop(value)) return std::nullopt;
        return std::optional<T>{std::move(value)};
    }

    // ── Introspection (any thread, approximate) ───────────────────────────────

    /// Approximate number of items in the queue (may be stale by the time you use it).
    [[nodiscard]] std::size_t size_approx() const noexcept {
        const std::size_t h = m_head.load(std::memory_order_relaxed);
        const std::size_t t = m_tail.load(std::memory_order_relaxed);
        return h - t;  // wraps correctly for unsigned arithmetic
    }

    [[nodiscard]] bool empty_approx() const noexcept { return size_approx() == 0; }
    [[nodiscard]] bool full_approx()  const noexcept { return size_approx() == Capacity; }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    template<typename U>
    [[nodiscard]] bool push_impl(U&& item) noexcept {
        const std::size_t head = m_head.load(std::memory_order_relaxed);
        const std::size_t next = head + 1;

        // Acquire: see all consumer's tail updates.
        if ((next & kMask) == (m_tail.load(std::memory_order_acquire) & kMask)
            && next != m_tail.load(std::memory_order_relaxed))
            return false;  // full

        // Simpler full check (works because Capacity is power-of-2):
        // full when (head+1 - tail) == Capacity
        if (head - m_tail.load(std::memory_order_acquire) == Capacity - 1)
            return false;

        m_buffer[head & kMask] = std::forward<U>(item);

        // Release: consumer can now see the new item.
        m_head.store(next, std::memory_order_release);
        return true;
    }

    // Each atomic on its own cache line — prevents false sharing.
    // Producer writes m_head; consumer writes m_tail.
    // Without padding: both fit on the same cache line → cache ping-pong.
    alignas(64) std::atomic<std::size_t> m_head{0};  // written by producer
    alignas(64) std::atomic<std::size_t> m_tail{0};  // written by consumer

    // The ring buffer — between the two atomics to avoid sharing
    std::array<T, Capacity> m_buffer{};
};
