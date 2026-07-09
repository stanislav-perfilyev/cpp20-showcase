#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 03_coroutines/generator.h  —  C++20 co_yield Generator<T>
//
// A minimal but correct eager-pull generator.
// Each operator++ resumes the coroutine until the next co_yield.
//
// Usage:
//   Generator<int> fib = fibonacci();
//   for (int n : fib | std::views::take(10)) { ... }
// ─────────────────────────────────────────────────────────────────────────────
#include <concepts>
#include <coroutine>
#include <exception>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <utility>

template<std::movable T>
/// Generator<T> — C++20 co_yield generator; input-range compatible.
class Generator {
public:
    // ── Promise type (required by coroutine machinery) ────────────────────────
    /// promise_type — coroutine promise: stores current value and exception.
    struct promise_type {
        std::optional<T>   current_value;
        std::exception_ptr exception;

        Generator get_return_object() noexcept {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }

        std::suspend_always yield_value(T value) noexcept {
            current_value = std::move(value);
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() { exception = std::current_exception(); }
    };

    // ── Iterator (input iterator — single-pass) ────────────────────────────────
    /// iterator — input iterator that resumes the coroutine on each increment.
    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = const T*;
        using reference         = const T&;

        explicit iterator(std::coroutine_handle<promise_type> h) : m_handle(h) {
            advance();  // prime: move to first value
        }
        // Sentinel constructor
        iterator() = default;

        reference operator*()  const { return *m_handle.promise().current_value; }
        pointer   operator->() const { return &*m_handle.promise().current_value; }

        iterator& operator++() {
            advance();
            return *this;
        }
        iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        bool operator==(std::default_sentinel_t) const noexcept {
            return !m_handle || m_handle.done();
        }

    private:
        void advance() {
            if (!m_handle || m_handle.done()) return;
            m_handle.resume();
            if (m_handle.promise().exception)
                std::rethrow_exception(m_handle.promise().exception);
        }
        std::coroutine_handle<promise_type> m_handle;
    };

    // ── Range interface ───────────────────────────────────────────────────────
    [[nodiscard]] iterator begin() {
        return iterator{m_handle};
    }
    [[nodiscard]] std::default_sentinel_t end() const noexcept {
        return {};
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    Generator() = default;
    Generator(Generator&& other) noexcept : m_handle(std::exchange(other.m_handle, {})) {}
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (m_handle) m_handle.destroy();
            m_handle = std::exchange(other.m_handle, {});
        }
        return *this;
    }
    ~Generator() { if (m_handle) m_handle.destroy(); }

    // Non-copyable (coroutine handle is unique resource)
    Generator(const Generator&)            = delete;
    Generator& operator=(const Generator&) = delete;

private:
    explicit Generator(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    std::coroutine_handle<promise_type> m_handle;
};

// ── Example generators ────────────────────────────────────────────────────────

/// Infinite Fibonacci sequence: 0, 1, 1, 2, 3, 5, 8, ...
[[nodiscard]] inline Generator<long long> fibonacci() {
    long long a = 0, b = 1;
    while (true) {
        co_yield a;
        auto tmp = a + b;
        a = b;
        b = tmp;
    }
}

// Internal coroutine — called only after step is validated.
[[nodiscard]] inline Generator<long long> irange_impl(long long start, long long stop, long long step) {
    for (long long i = start; (step > 0) ? (i < stop) : (i > stop); i += step)
        co_yield i;
}

/// Range generator [start, stop) with step.
/// Throws std::invalid_argument immediately (not lazily) if step == 0.
/// Note: irange itself is NOT a coroutine so the throw is visible to EXPECT_THROW.
[[nodiscard]] inline Generator<long long> irange(long long start, long long stop, long long step = 1) {
    if (step == 0) throw std::invalid_argument("irange: step must not be zero");
    return irange_impl(start, stop, step);
}

/// Generator from any input range — yields each element in order.
/// Useful for adapting existing containers/ranges to Generator pipelines.
template<std::ranges::input_range R>
[[nodiscard]] inline Generator<std::ranges::range_value_t<R>>
from_range(const R& rng) {
    for (const auto& v : rng)
        co_yield v;
}
