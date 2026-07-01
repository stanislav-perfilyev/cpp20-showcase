add_executable(test_mdspan test_mdspan.cpp)
target_compile_features(test_mdspan PRIVATE cxx_std_23)
target_link_libraries(test_mdspan PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_mdspan)
