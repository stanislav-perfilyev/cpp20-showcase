add_executable(test_jthread test_jthread.cpp)
target_compile_features(test_jthread PRIVATE cxx_std_23)
target_link_libraries(test_jthread PRIVATE GTest::gtest GTest::gtest_main)
# Link pthreads on Linux
find_package(Threads REQUIRED)
target_link_libraries(test_jthread PRIVATE Threads::Threads)
gtest_discover_tests(test_jthread)
