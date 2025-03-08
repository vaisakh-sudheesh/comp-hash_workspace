cmake_minimum_required(VERSION 3.20)
project (CompressionHashing_Testbed)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

## Include the dependencies
FetchContent_Declare(googletest SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest-1.16.0 )
FetchContent_Declare(googlebenchmark SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/benchmark-1.9.1)

FetchContent_MakeAvailable(googletest googlebenchmark)
FetchContent_MakeAvailable()

add_custom_target(deps_xxHash
            COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxHash-0.8.3 prefix=${CMAKE_BINARY_DIR} install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxHash-0.8.3
            COMMENT "Building xxHash"
        )

add_custom_target(deps_lzbench
            COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/lzbench-2.0.1 prefix=${CMAKE_BINARY_DIR}  install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/lzbench-2.0.1
            COMMENT "Building lzbench"
        )

enable_testing()
add_executable(CompressionHashing_Testbed
    tests/test_helloworld.cpp)

add_dependencies(CompressionHashing_Testbed deps_xxHash deps_lzbench)