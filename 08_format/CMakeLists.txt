add_executable(test_format test_format.cpp)
target_compile_features(test_format PRIVATE cxx_std_23)
target_link_libraries(test_format PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_format)
