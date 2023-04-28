#include <benchmark/benchmark.h>
#include "locol.hpp"
#include "9z_zero/dsecs_9z.hpp"

template<BenchmarkSettings bs>
static void locol9z_A(benchmark::State& state) {
    locol_bm_A<bs, dsecs9z::World>(state);
}

BENCHMARK(locol9z_A<BsUpdate>);
BENCHMARK(locol9z_A<BsInit>);
BENCHMARK(locol9z_A<BsExpand>);
BENCHMARK(locol9z_A<BsChurn>);
