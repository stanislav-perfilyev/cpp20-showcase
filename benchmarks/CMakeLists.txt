add_executable(bench_queue bench_queue.cpp)
target_compile_features(bench_queue PRIVATE cxx_std_23)
target_link_libraries(bench_queue PRIVATE benchmark::benchmark)
find_package(Threads REQUIRED)
target_link_libraries(bench_queue PRIVATE Threads::Threads)
# Benchmarks not added to ctest (slow); run manually: ./build/release/benchmarks/bench_queue
