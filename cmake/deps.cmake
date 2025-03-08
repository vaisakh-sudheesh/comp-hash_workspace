include(FetchContent)

set (BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
set (BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Suppressing benchmark's gtest tests" FORCE)
set (BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "Suppressing benchmark's bundled gtest" FORCE)
set (BENCHMARK_FORCE_WERROR OFF CACHE BOOL "Suppressing benchmark's Werror" FORCE)
set (BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "Suppressing benchmark's docs" FORCE)


## Include the dependencies
FetchContent_Declare(googletest SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest-1.16.0 )
FetchContent_Declare(googlebenchmark SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/benchmark-1.9.1)

FetchContent_MakeAvailable(googletest googlebenchmark)

add_custom_target(deps_xxHash
            COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxHash-0.8.3 prefix=${CMAKE_BINARY_DIR} install -j8
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxHash-0.8.3
            COMMENT "Building xxHash."
        )

add_custom_target(deps_lzbench
            ${CMAKE_COMMAND} -E env LIBRARY_BUILD=1 make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/lzbench-2.0.1 clean 
            COMMAND ${CMAKE_COMMAND} -E env LIBRARY_BUILD=1 make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/lzbench-2.0.1 prefix=${CMAKE_BINARY_DIR} -j8 install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/lzbench-2.0.1
            COMMENT "Building lzbench as library."
        )
