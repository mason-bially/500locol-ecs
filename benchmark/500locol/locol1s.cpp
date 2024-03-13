#include <benchmark/benchmark.h>
#include "locol.hpp"
#include "1s_ergonomics1/dsecs_1s.hpp"

template<BenchmarkSettings bs>
static void locol1s_A(benchmark::State& state) {
    locol_bm_A<bs, dsecs1s::World>(state);
}

BENCHMARK(locol1s_A<BsUpdate>);
BENCHMARK(locol1s_A<BsInit>);
BENCHMARK(locol1s_A<BsExpand>)->Iterations(BMChurnIter);
BENCHMARK(locol1s_A<BsChurn>)->Iterations(BMChurnIter);
