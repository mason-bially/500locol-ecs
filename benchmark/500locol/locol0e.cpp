#include <benchmark/benchmark.h>
#include "locol.hpp"
#include "0e_ergonomics1/dsecs_0e.hpp"

template<BenchmarkSettings bs>
static void locol0e_A(benchmark::State& state) {
    locol_bm_A<bs, dsecs0e::World>(state);
}

BENCHMARK(locol0e_A<BsUpdate>);
BENCHMARK(locol0e_A<BsInit>);
BENCHMARK(locol0e_A<BsExpand>)->Iterations(BMChurnIter);
BENCHMARK(locol0e_A<BsChurn>)->Iterations(BMChurnIter);
