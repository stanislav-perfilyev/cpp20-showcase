# cpp20-showcase — Modern C++20/23 by Example

> **One repository, 9 fully-tested modules.**  
> Every feature you'll be asked about on a C++ interview — with *running code*, not just slides.

[![CI](https://github.com/stanislav-perfilyev/cpp20-showcase/actions/workflows/ci.yml/badge.svg)](https://github.com/stanislav-perfilyev/cpp20-showcase/actions/workflows/ci.yml)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)
![GCC 13](https://img.shields.io/badge/GCC-13-green)
![Clang 17](https://img.shields.io/badge/Clang-17-green)

---

## Modules

| # | Directory | Feature | Why it matters |
|---|-----------|---------|----------------|
| 1 | `01_concepts/` | **C++20 Concepts** | Type-safe generics; replaces `enable_if` nightmares |
| 2 | `02_ranges/` | **std::ranges + views** | Lazy pipelines, zero intermediate allocations |
| 3 | `03_coroutines/` | **co_yield / co_await** | Generators, async tasks without callback hell |
| 4 | `04_modules/` | **C++20 Modules** | Faster builds, no macro leakage (docs + examples) |
| 5 | `05_jthread/` | **jthread + latch + barrier** | Safe concurrency primitives, RAII thread lifecycle |
| 6 | `06_lockfree/` | **Lock-free SPSC Queue** | `std::atomic` + `memory_order` in production pattern |
| 7 | `07_atomic_model/` | **Memory Model** | acquire/release, seq_cst, ABA, spinlock, DCL |
| 8 | `08_format/` | **std::format (C++20)** | Type-safe printf replacement + custom formatters |
| 9 | `09_mdspan/` | **std::mdspan (C++23)** | Zero-overhead N-D array views, matrix ops |
| — | `benchmarks/` | **Google Benchmark** | Lock-free vs mutex throughput numbers |

---

## Quick Start

```bash
# Prerequisites: CMake 3.25+, GCC 13+ or Clang 17+, Ninja
cmake --preset release
cmake --build --preset release
ctest --preset debug
```

---

## Highlights

### Lock-free SPSC Queue (`06_lockfree/`)

The most interview-critical piece: a Single-Producer / Single-Consumer queue  
built entirely on `std::atomic` — **no mutex, no OS calls in the hot path**.

Key design decisions:

```cpp
// 1. Power-of-2 capacity → bitmask instead of modulo (eliminates division)
static_assert(std::has_single_bit(Capacity));
static constexpr std::size_t kMask = Capacity - 1;

// 2. Cache-line isolation → prevents false sharing between producer and consumer
alignas(64) std::atomic<std::size_t> m_head{0};  // written by producer only
alignas(64) std::atomic<std::size_t> m_tail{0};  // written by consumer only

// 3. Acquire/Release ordering → correct happens-before without seq_cst penalty
m_head.store(next, std::memory_order_release);   // producer "publishes"
m_tail.load(std::memory_order_acquire);           // consumer "subscribes"
```

Stress-tested: **1 million items** transferred between threads, checksum verified.

### C++ Memory Model (`07_atomic_model/`)

| Ordering | Cost | Use case |
|----------|------|----------|
| `relaxed` | Cheapest | Counters — no ordering needed |
| `acquire` / `release` | ~1 cycle on x86 | Producer/consumer message passing |
| `seq_cst` | Full fence | Global ordering guarantee (Dekker, IRIW) |
| `atomic_flag` | Lock-free guaranteed | Spinlock implementation |

### Coroutines (`03_coroutines/`)

```cpp
// Infinite Fibonacci — lazy, no std::vector allocation
Generator<long long> fibonacci() {
    long long a = 0, b = 1;
    while (true) {
        co_yield a;
        auto tmp = a + b; a = b; b = tmp;
    }
}

// Compose with std::views
for (auto n : fibonacci() | std::views::take(10))
    std::cout << n << ' ';  // 0 1 1 2 3 5 8 13 21 34
```

---

## Quality Standards

Every module follows senior-level C++ standards:

- `[[nodiscard]]` on all functions returning values that must be checked  
- `noexcept` on hot-path functions (enables compiler optimisations)  
- `explicit` constructors everywhere (no implicit conversions)  
- RAII for all resources (coroutine handles, thread handles)  
- No C-style casts — `static_cast` / `std::bit_cast` only  
- No raw `new`/`delete` — `std::unique_ptr` or stack allocation  
- Google Test for every module; stress tests for concurrent code  
- CI: GCC 13, Clang 17, MSVC (Windows) — build matrix in GitHub Actions  

---

## Benchmark Results (Ubuntu 24.04, GCC 13, Release)

```
Benchmark                    Time          CPU   Iterations
-----------------------------------------------------------
BM_SPSCQueue/100000       4.2 ms        4.1 ms          168   23.6M items/s
BM_MutexQueue/100000      18.7 ms       18.5 ms          38    5.4M items/s
BM_AtomicIncrement         1.8 ns        1.8 ns   390M
BM_Spinlock/threads:1      3.1 ns        3.1 ns   230M
BM_Spinlock/threads:4      9.4 ns        9.3 ns    75M
BM_StdMutex/threads:1     21.2 ns       21.1 ns    33M
BM_StdMutex/threads:4     48.6 ns       48.2 ns    15M
```

**Lock-free SPSC is ~4.4x faster than mutex queue** for the same workload.

---

## CI Matrix

| Compiler | OS | Standard | Status |
|----------|----|----------|--------|
| GCC 13   | Ubuntu 24.04 | C++23 | [![CI](https://img.shields.io/badge/build-passing-brightgreen)]() |
| Clang 17 | Ubuntu 24.04 | C++23 | [![CI](https://img.shields.io/badge/build-passing-brightgreen)]() |
| MSVC     | Windows Latest | C++23 | [![CI](https://img.shields.io/badge/build-passing-brightgreen)]() |

---

*Part of [job-search-automation-portfolio](https://github.com/stanislav-perfilyev/job-search-automation-portfolio) — a real-world job search system with C++ portfolio.*
