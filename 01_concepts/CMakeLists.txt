add_executable(test_concepts test_concepts.cpp)
target_compile_features(test_concepts PRIVATE cxx_std_23)
target_link_libraries(test_concepts PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_concepts)
