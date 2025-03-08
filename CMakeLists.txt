cmake_minimum_required(VERSION 3.20)
project (CompressionHashing_Testbed)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps.cmake)

enable_testing()
add_executable(CompressionHashing_Testbed
    tests/test_helloworld.cpp)
target_link_libraries(CompressionHashing_Testbed gtest_main benchmark::benchmark)

add_dependencies(CompressionHashing_Testbed deps_xxHash deps_lzbench)