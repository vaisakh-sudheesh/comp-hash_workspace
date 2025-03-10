cmake_minimum_required(VERSION 3.20)
project (CompressionHashing_Testbed)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps.cmake)

include_directories(${CMAKE_BINARY_DIR}/include)
enable_testing()
add_executable(CompressionHashing_Testbed
    tests/test_xxhash.cpp)
target_compile_definitions(CompressionHashing_Testbed PRIVATE LIBRARY_BUILD)
target_link_libraries(CompressionHashing_Testbed gtest_main benchmark::benchmark lzbench xxhash)

add_dependencies(CompressionHashing_Testbed all_deps)