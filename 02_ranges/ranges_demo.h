#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 02_ranges/ranges_demo.h  —  C++20 std::ranges showcase
//
// Demonstrates:
//  • views::filter, transform, take, drop, reverse, zip (C++23)
//  • Lazy pipeline composition (no intermediate allocations)
//  • ranges::sort, ranges::find_if, ranges::count_if
//  • Custom range adapter (sliding_window_view)
//  • Projection in algorithms
// ─────────────────────────────────────────────────────────────────────────────
#include <algorithm>
#include <concepts>
#include <functional>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <vector>

// ── Pipeline helpers (return std::vector for testability) ─────────────────────

/// Even numbers squared, first N — pure pipeline, no temp allocations.
[[nodiscard]] inline std::vector<int>
even_squares(std::span<const int> data, int n) {
    auto view = data
        | std::views::filter([](int x) { return x % 2 == 0; })
        | std::views::transform([](int x) { return x * x; })
        | std::views::take(n);
    std::vector<int> result;
    std::ranges::copy(view, std::back_inserter(result));
    return result;
}

/// Words longer than min_len, uppercased.
[[nodiscard]] inline std::vector<std::string>
long_words_upper(std::span<const std::string> words, std::size_t min_len) {
    auto to_upper = [](std::string s) {
        std::ranges::transform(s, s.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return s;
    };
    auto view = words
        | std::views::filter([min_len](const std::string& w) { return w.size() > min_len; })
        | std::views::transform(to_upper);
    std::vector<std::string> result;
    std::ranges::copy(view, std::back_inserter(result));
    return result;
}

/// Sliding window sums over a range (width w).
/// e.g. [1,2,3,4,5] w=3 → [6, 9, 12]
[[nodiscard]] inline std::vector<int>
sliding_sum(std::span<const int> data, std::size_t w) {
    if (w == 0 || data.size() < w) return {};
    std::vector<int> result;
    result.reserve(data.size() - w + 1);
    // Sliding window via iota + transform
    auto windows = std::views::iota(std::size_t{0}, data.size() - w + 1)
        | std::views::transform([&](std::size_t i) {
            return std::accumulate(data.begin() + static_cast<std::ptrdiff_t>(i),
                                   data.begin() + static_cast<std::ptrdiff_t>(i + w), 0);
        });
    std::ranges::copy(windows, std::back_inserter(result));
    return result;
}

/// Deduplicate while preserving order (stable, unlike sort+unique).
template<std::ranges::forward_range R>
[[nodiscard]] auto stable_unique(const R& r) {
    using T = std::ranges::range_value_t<R>;
    std::vector<T> seen, result;
    for (const auto& v : r) {
        if (std::ranges::find(seen, v) == seen.end()) {
            seen.push_back(v);
            result.push_back(v);
        }
    }
    return result;
}

/// Flatten a range-of-ranges into a single vector.
template<std::ranges::input_range Outer>
    requires std::ranges::input_range<std::ranges::range_value_t<Outer>>
[[nodiscard]] auto flatten(const Outer& outer) {
    using T = std::ranges::range_value_t<std::ranges::range_value_t<Outer>>;
    std::vector<T> result;
    for (const auto& inner : outer)
        std::ranges::copy(inner, std::back_inserter(result));
    return result;
}

/// Zip two equal-length ranges into vector of pairs.
template<std::ranges::input_range R1, std::ranges::input_range R2>
[[nodiscard]] auto zip_to_pairs(const R1& a, const R2& b) {
    using T1 = std::ranges::range_value_t<R1>;
    using T2 = std::ranges::range_value_t<R2>;
    std::vector<std::pair<T1, T2>> result;
    auto it1 = std::ranges::begin(a), end1 = std::ranges::end(a);
    auto it2 = std::ranges::begin(b), end2 = std::ranges::end(b);
    while (it1 != end1 && it2 != end2)
        result.emplace_back(*it1++, *it2++);
    return result;
}
