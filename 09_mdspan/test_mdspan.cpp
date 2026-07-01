// ─────────────────────────────────────────────────────────────────────────────
// 09_mdspan/test_mdspan.cpp  —  C++23 std::mdspan showcase + tests
//
// std::mdspan = non-owning multidimensional view over contiguous memory.
// Zero-overhead abstraction: no allocation, compiler optimises as well as raw[].
// ─────────────────────────────────────────────────────────────────────────────
#include <gtest/gtest.h>
#include <mdspan>
#include <array>
#include <numeric>
#include <vector>

// ── Matrix multiply using mdspan ──────────────────────────────────────────────

/// Matrix-vector multiply: result = A * x
/// A is (rows x cols), x is (cols), result is (rows)
template<typename T>
void matvec(std::mdspan<const T, std::dextents<std::size_t, 2>> A,
            std::mdspan<const T, std::dextents<std::size_t, 1>> x,
            std::mdspan<T,       std::dextents<std::size_t, 1>> result)
{
    for (std::size_t i = 0; i < A.extent(0); ++i) {
        result[i] = T{};
        for (std::size_t j = 0; j < A.extent(1); ++j)
            result[i] += A[i, j] * x[j];
    }
}

/// Transpose A into B (both must have same extents in swapped order)
template<typename T>
void transpose(std::mdspan<const T, std::dextents<std::size_t, 2>> A,
               std::mdspan<T,       std::dextents<std::size_t, 2>> B)
{
    for (std::size_t i = 0; i < A.extent(0); ++i)
        for (std::size_t j = 0; j < A.extent(1); ++j)
            B[j, i] = A[i, j];
}

/// Frobenius norm of a 2D matrix
template<typename T>
[[nodiscard]] T frobenius(std::mdspan<const T, std::dextents<std::size_t, 2>> A) {
    T sum{};
    for (std::size_t i = 0; i < A.extent(0); ++i)
        for (std::size_t j = 0; j < A.extent(1); ++j)
            sum += A[i, j] * A[i, j];
    return sum;  // return sum-of-squares (caller takes sqrt if needed)
}

// ── Tests ─────────────────────────────────────────────────────────────────────
TEST(Mdspan, Basic2D_AccessAndExtent) {
    std::array<int, 6> data{1,2,3,4,5,6};
    auto m = std::mdspan(data.data(), 2, 3);  // 2×3 matrix
    EXPECT_EQ(m.extent(0), 2u);
    EXPECT_EQ(m.extent(1), 3u);
    EXPECT_EQ((m[0, 0]), 1);
    EXPECT_EQ((m[1, 2]), 6);
}

TEST(Mdspan, MatVec_Identity) {
    // Identity 3×3 * [1,2,3] = [1,2,3]
    std::vector<double> A_data{1,0,0, 0,1,0, 0,0,1};
    std::vector<double> x_data{1,2,3};
    std::vector<double> r_data(3, 0.0);

    auto A = std::mdspan(A_data.data(), 3, 3);
    auto x = std::mdspan(const_cast<const double*>(x_data.data()), 3);
    auto r = std::mdspan(r_data.data(), 3);

    matvec<double>(A, x, r);

    EXPECT_DOUBLE_EQ(r_data[0], 1.0);
    EXPECT_DOUBLE_EQ(r_data[1], 2.0);
    EXPECT_DOUBLE_EQ(r_data[2], 3.0);
}

TEST(Mdspan, Transpose_2x3_to_3x2) {
    std::array<int, 6> A_data{1,2,3,4,5,6};
    std::array<int, 6> B_data{};

    auto A = std::mdspan(const_cast<const int*>(A_data.data()), 2, 3);
    auto B = std::mdspan(B_data.data(), 3, 2);

    transpose<int>(A, B);

    // A[0,0]=1 → B[0,0]=1; A[0,1]=2 → B[1,0]=2; A[1,0]=4 → B[0,1]=4
    EXPECT_EQ((B[0, 0]), 1);
    EXPECT_EQ((B[1, 0]), 2);
    EXPECT_EQ((B[0, 1]), 4);
}

TEST(Mdspan, FrobeniusNorm) {
    // [[1,2],[3,4]] → 1+4+9+16 = 30
    std::array<int, 4> data{1,2,3,4};
    auto m = std::mdspan(const_cast<const int*>(data.data()), 2, 2);
    EXPECT_EQ(frobenius<int>(m), 30);
}

TEST(Mdspan, 3D_Tensor) {
    std::vector<float> t(24);
    std::iota(t.begin(), t.end(), 0.0f);
    auto tensor = std::mdspan(t.data(), 2, 3, 4);  // 2×3×4
    EXPECT_EQ(tensor.rank(), 3u);
    EXPECT_EQ(tensor.size(), 24u);
    EXPECT_FLOAT_EQ((tensor[1, 2, 3]), 23.0f);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
