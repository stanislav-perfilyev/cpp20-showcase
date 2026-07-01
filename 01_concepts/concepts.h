#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 01_concepts/concepts.h  —  C++20 Concepts showcase
//
// Demonstrates:
//  • Basic concepts: Numeric, Printable, Sortable, Hashable
//  • Compound concept: Container
//  • Concept-constrained functions and class templates
//  • requires expressions (simple + compound + nested)
//  • Concept subsumption (more-constrained overload wins)
// ─────────────────────────────────────────────────────────────────────────────
#include <concepts>
#include <functional>
#include <iostream>
#include <iterator>
#include <ostream>
#include <ranges>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <vector>

// ── Primitive concepts ────────────────────────────────────────────────────────

// Numeric: integer or floating-point (std::integral / std::floating_point)
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// Printable: can be streamed to std::ostream
template<typename T>
concept Printable = requires(std::ostream& os, const T& t) {
    { os << t } -> std::same_as<std::ostream&>;
};

// EqualityComparable + less-than: subset of std::totally_ordered
template<typename T>
concept Sortable = std::totally_ordered<T>;

// Hashable: std::hash<T> is well-formed
template<typename T>
concept Hashable = requires(const T& t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

// Container concept: has begin/end, size, value_type
template<typename C>
concept Container = requires(C c) {
    typename C::value_type;
    { c.begin()  } -> std::input_iterator;
    { c.end()    } -> std::input_iterator;
    { c.size()   } -> std::convertible_to<std::size_t>;
    { c.empty()  } -> std::same_as<bool>;
};

// ── Concept-constrained functions ─────────────────────────────────────────────

// Overload set: subsumption picks the most-constrained overload.
// Numeric + Printable wins over just Printable.
template<Printable T>
void display(const T& v, std::ostream& os = std::cout) {
    os << "[Printable] " << v << "\n";
}

template<Numeric T>          // Numeric implies Printable for built-in types
    requires Printable<T>
void display(const T& v, std::ostream& os = std::cout) {
    os << "[Numeric+Printable] " << v << "\n";
}

// clamp() only for types that are totally ordered AND numeric
template<Numeric T>
    requires Sortable<T>
[[nodiscard]] constexpr T clamp(T value, T lo, T hi) noexcept {
    return value < lo ? lo : (value > hi ? hi : value);
}

// sum() for any container of numerics — returns element type
template<Container C>
    requires Numeric<typename C::value_type>
[[nodiscard]] auto sum(const C& c) noexcept {
    typename C::value_type acc{};
    for (const auto& v : c) acc += v;
    return acc;
}

// ── Type-safe container (concept-constrained class template) ──────────────────

/// A minimal type-safe stack that accepts only Sortable + Printable elements.
/// [[nodiscard]] on all accessors (senior standard).
template<typename T>
    requires (Sortable<T> && Printable<T>)
class TypedStack {
public:
    using value_type = T;

    void push(T value) { m_data.push_back(std::move(value)); }

    [[nodiscard]] const T& top() const {
        if (m_data.empty()) throw std::runtime_error("TypedStack::top on empty stack");
        return m_data.back();
    }

    void pop() {
        if (m_data.empty()) throw std::runtime_error("TypedStack::pop on empty stack");
        m_data.pop_back();
    }

    [[nodiscard]] bool   empty() const noexcept { return m_data.empty(); }
    [[nodiscard]] size_t size()  const noexcept { return m_data.size();  }

    /// Returns the minimum element without removing it.
    [[nodiscard]] const T& min() const {
        if (m_data.empty()) throw std::runtime_error("TypedStack::min on empty stack");
        return *std::ranges::min_element(m_data);
    }

    /// Returns the maximum element without removing it.
    [[nodiscard]] const T& max() const {
        if (m_data.empty()) throw std::runtime_error("TypedStack::max on empty stack");
        return *std::ranges::max_element(m_data);
    }

    /// Print all elements to stream.
    void print(std::ostream& os = std::cout) const {
        os << "TypedStack[";
        for (size_t i = 0; i < m_data.size(); ++i) {
            os << m_data[i];
            if (i + 1 < m_data.size()) os << ", ";
        }
        os << "]\n";
    }

private:
    std::vector<T> m_data;
};
