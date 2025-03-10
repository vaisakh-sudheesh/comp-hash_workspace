#include <benchmark/benchmark.h>

#include <stdio.h>
#include <iostream>
#include <lzbench/lzbench-lib.h>

#include <xxhash.h>



static void xxHashBM (benchmark::State& state) {
    
}

BENCHMARK(xxHashBM);



// Run the benchmark
BENCHMARK_MAIN();