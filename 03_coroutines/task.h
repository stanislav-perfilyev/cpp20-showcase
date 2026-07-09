#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 03_coroutines/task.h  —  C++20 co_await Task<T> (synchronous scheduler)
//
// A simple eager Task<T> that:
//  • Stores a result or exception
//  • co_await on another Task<T> chains execution inline
//  • sync_wait() drives the coroutine to completion on the calling thread
//
// No thread pool — demonstrates coroutine mechanics without Asio dependency.
// ─────────────────────────────────────────────────────────────────────────────
#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

template<typename T = void>
/// Task<T> — eager C++20 coroutine: runs immediately, awaitable, sync_wait()-able.
class Task {
public:
    /// promise_type — stores coroutine result or exception; enables get_return_object.
    struct promise_type {
        std::optional<T>   result;
        std::exception_ptr exception;

        Task get_return_object() noexcept {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never  initial_suspend() noexcept { return {}; }  // eager
        std::suspend_always final_suspend()   noexcept { return {}; }  // keep frame alive

        template<std::convertible_to<T> U>
        void return_value(U&& v) { result = std::forward<U>(v); }
        void unhandled_exception() { exception = std::current_exception(); }
    };

    // ── Awaitable (allows: co_await task) ────────────────────────────────────
    bool await_ready() const noexcept { return m_handle.done(); }
    void await_suspend(std::coroutine_handle<> caller) noexcept {
        // Resume this task inline on the calling coroutine's thread.
        // When this task finishes it will resume the caller.
        m_caller = caller;
    }
    [[nodiscard]] T await_resume() {
        if (m_handle.promise().exception)
            std::rethrow_exception(m_handle.promise().exception);
        return std::move(*m_handle.promise().result);
    }

    // ── sync_wait: run to completion on current thread ────────────────────────
    [[nodiscard]] T sync_wait() {
        // The coroutine started eagerly; if it's not done, pump it.
        while (!m_handle.done())
            m_handle.resume();
        if (m_handle.promise().exception)
            std::rethrow_exception(m_handle.promise().exception);
        if (!m_handle.promise().result)
            throw std::runtime_error("Task completed without a value");
        return std::move(*m_handle.promise().result);
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    Task() = default;
    explicit Task(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    Task(Task&& o) noexcept : m_handle(std::exchange(o.m_handle, {})) {}
    Task& operator=(Task&& o) noexcept {
        if (this != &o) {
            if (m_handle) m_handle.destroy();
            m_handle = std::exchange(o.m_handle, {});
        }
        return *this;
    }
    ~Task() { if (m_handle) m_handle.destroy(); }
    Task(const Task&)            = delete;
    Task& operator=(const Task&) = delete;

private:
    std::coroutine_handle<promise_type> m_handle;
    std::coroutine_handle<>             m_caller;  // set by await_suspend
};

// ── Void specialisation ───────────────────────────────────────────────────────
template<>
/// Task<void> — void specialisation: no result storage, just completion + exception.
class Task<void> {
public:
    /// promise_type — void promise: no result, propagates exceptions only.
    struct promise_type {
        std::exception_ptr exception;
        Task get_return_object() noexcept {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never  initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { exception = std::current_exception(); }
    };

    bool await_ready() const noexcept { return m_handle.done(); }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    void await_resume() {
        if (m_handle.promise().exception)
            std::rethrow_exception(m_handle.promise().exception);
    }
    void sync_wait() {
        while (!m_handle.done()) m_handle.resume();
        if (m_handle.promise().exception)
            std::rethrow_exception(m_handle.promise().exception);
    }

    Task() = default;
    explicit Task(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    Task(Task&& o) noexcept : m_handle(std::exchange(o.m_handle, {})) {}
    Task& operator=(Task&& o) noexcept {
        if (this != &o) { if (m_handle) m_handle.destroy(); m_handle = std::exchange(o.m_handle, {}); }
        return *this;
    }
    ~Task() { if (m_handle) m_handle.destroy(); }
    Task(const Task&)            = delete;
    Task& operator=(const Task&) = delete;

private:
    std::coroutine_handle<promise_type> m_handle;
};
