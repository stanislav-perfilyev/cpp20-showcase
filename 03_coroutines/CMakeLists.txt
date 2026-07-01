add_executable(test_coroutines test_coroutines.cpp)
target_compile_features(test_coroutines PRIVATE cxx_std_23)
target_link_libraries(test_coroutines PRIVATE GTest::gtest GTest::gtest_main)
# Coroutines require explicit flag on GCC < 13
target_compile_options(test_coroutines PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>)
gtest_discover_tests(test_coroutines)
