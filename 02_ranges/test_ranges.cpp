// ─────────────────────────────────────────────────────────────────────────────
// 02_ranges/test_ranges.cpp  —  Google Test for C++20 Ranges
// ─────────────────────────────────────────────────────────────────────────────
#include "ranges_demo.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

// ── even_squares ──────────────────────────────────────────────────────────────
TEST(Ranges, EvenSquares_Basic) {
    std::vector<int> data{1,2,3,4,5,6,7,8};
    auto r = even_squares(data, 3);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 4);   // 2^2
    EXPECT_EQ(r[1], 16);  // 4^2
    EXPECT_EQ(r[2], 36);  // 6^2
}
TEST(Ranges, EvenSquares_EmptyInput) {
    std::vector<int> data{};
    EXPECT_TRUE(even_squares(data, 5).empty());
}
TEST(Ranges, EvenSquares_TakeMoreThanAvailable) {
    std::vector<int> data{2,4};
    auto r = even_squares(data, 100);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0], 4);
    EXPECT_EQ(r[1], 16);
}

// ── long_words_upper ──────────────────────────────────────────────────────────
TEST(Ranges, LongWordsUpper_FiltersAndUppercases) {
    std::vector<std::string> words{"hi", "hello", "world", "ok", "cpp"};
    auto r = long_words_upper(words, 3);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0], "HELLO");
    EXPECT_EQ(r[1], "WORLD");
}
TEST(Ranges, LongWordsUpper_EmptyResult) {
    std::vector<std::string> words{"a", "bb"};
    EXPECT_TRUE(long_words_upper(words, 5).empty());
}

// ── sliding_sum ───────────────────────────────────────────────────────────────
TEST(Ranges, SlidingSum_Basic) {
    std::vector<int> data{1,2,3,4,5};
    auto r = sliding_sum(data, 3);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 6);   // 1+2+3
    EXPECT_EQ(r[1], 9);   // 2+3+4
    EXPECT_EQ(r[2], 12);  // 3+4+5
}
TEST(Ranges, SlidingSum_WindowLargerThanData) {
    std::vector<int> data{1,2};
    EXPECT_TRUE(sliding_sum(data, 5).empty());
}
TEST(Ranges, SlidingSum_WindowEqualsSize) {
    std::vector<int> data{1,2,3};
    auto r = sliding_sum(data, 3);
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(r[0], 6);
}

// ── stable_unique ─────────────────────────────────────────────────────────────
TEST(Ranges, StableUnique_PreservesOrder) {
    std::vector<int> v{3,1,4,1,5,9,2,6,5,3};
    auto r = stable_unique(v);
    std::vector<int> expected{3,1,4,5,9,2,6};
    EXPECT_EQ(r, expected);
}
TEST(Ranges, StableUnique_AllUnique) {
    std::vector<int> v{1,2,3};
    EXPECT_EQ(stable_unique(v), v);
}

// ── flatten ───────────────────────────────────────────────────────────────────
TEST(Ranges, Flatten_Basic) {
    std::vector<std::vector<int>> nested{{1,2},{3,4},{5}};
    auto r = flatten(nested);
    std::vector<int> expected{1,2,3,4,5};
    EXPECT_EQ(r, expected);
}

// ── zip_to_pairs ──────────────────────────────────────────────────────────────
TEST(Ranges, ZipToPairs_Basic) {
    std::vector<int>    a{1,2,3};
    std::vector<std::string> b{"one","two","three"};
    auto r = zip_to_pairs(a, b);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], (std::pair<int,std::string>{1,"one"}));
    EXPECT_EQ(r[2], (std::pair<int,std::string>{3,"three"}));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
