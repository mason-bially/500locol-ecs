#include "locol.hpp"
#include "01_trinity/dsecs_01.hpp"

static void locol01(benchmark::State& state) {
    locol_bm<dsecs::World>(state);
}

BENCHMARK(locol01);
