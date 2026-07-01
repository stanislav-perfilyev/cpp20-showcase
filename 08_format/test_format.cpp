// ─────────────────────────────────────────────────────────────────────────────
// 08_format/test_format.cpp  —  C++20 std::format showcase + tests
// ─────────────────────────────────────────────────────────────────────────────
#include <gtest/gtest.h>
#include <format>
#include <string>
#include <chrono>
#include <vector>
#include <sstream>

// ── Basic formatting ──────────────────────────────────────────────────────────
TEST(StdFormat, BasicInt) {
    EXPECT_EQ(std::format("{}", 42), "42");
}
TEST(StdFormat, FloatPrecision) {
    EXPECT_EQ(std::format("{:.2f}", 3.14159), "3.14");
}
TEST(StdFormat, Padding_RightAlign) {
    EXPECT_EQ(std::format("{:>10}", "hi"), "        hi");
}
TEST(StdFormat, Padding_LeftAlign) {
    EXPECT_EQ(std::format("{:<10}", "hi"), "hi        ");
}
TEST(StdFormat, Padding_Center) {
    EXPECT_EQ(std::format("{:^10}", "hi"), "    hi    ");
}
TEST(StdFormat, ZeroPadHex) {
    EXPECT_EQ(std::format("{:#010x}", 255), "0x000000ff");
}
TEST(StdFormat, MultpleArgs) {
    std::string s = std::format("Hello, {}! You are {} years old.", "Stan", 28);
    EXPECT_EQ(s, "Hello, Stan! You are 28 years old.");
}

// ── Custom formatter ──────────────────────────────────────────────────────────
struct Point { double x, y; };

template<>
struct std::formatter<Point> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({:.2f}, {:.2f})", p.x, p.y);
    }
};

TEST(StdFormat, CustomFormatter) {
    Point p{1.5, 2.75};
    EXPECT_EQ(std::format("{}", p), "(1.50, 2.75)");
}
TEST(StdFormat, CustomFormatter_InLargerString) {
    Point p{0.0, 1.0};
    EXPECT_EQ(std::format("Origin to {}", p), "Origin to (0.00, 1.00)");
}

// ── format_to for performance ─────────────────────────────────────────────────
TEST(StdFormat, FormatTo_OutputIterator) {
    std::string buf;
    buf.reserve(64);
    std::format_to(std::back_inserter(buf), "{} + {} = {}", 1, 2, 3);
    EXPECT_EQ(buf, "1 + 2 = 3");
}

// ── Table formatting utility (real-world use) ─────────────────────────────────
[[nodiscard]] inline std::string format_table(
    const std::vector<std::pair<std::string, int>>& rows)
{
    std::string out;
    out += std::format("{:<20} {:>10}\n", "Name", "Count");
    out += std::string(31, '-') + "\n";
    for (const auto& [name, count] : rows)
        out += std::format("{:<20} {:>10}\n", name, count);
    return out;
}

TEST(StdFormat, TableFormatting) {
    auto tbl = format_table({{"Alice", 42}, {"Bob", 7}});
    EXPECT_TRUE(tbl.find("Alice") != std::string::npos);
    EXPECT_TRUE(tbl.find("42") != std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
