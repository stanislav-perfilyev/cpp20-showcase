add_executable(test_ranges test_ranges.cpp)
target_compile_features(test_ranges PRIVATE cxx_std_23)
target_link_libraries(test_ranges PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_ranges)
