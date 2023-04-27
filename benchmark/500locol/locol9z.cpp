#include <benchmark/benchmark.h>
#include "locol.hpp"
#include "9z_zero/dsecs_9z.hpp"

static void locol9z(benchmark::State& state) {
    locol_bm<dsecs::World>(state);
}

BENCHMARK(locol9z);
