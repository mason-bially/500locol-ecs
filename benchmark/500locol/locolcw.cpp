#include <benchmark/benchmark.h>
#include "locol.hpp"
#include "dsecs.hpp"

template<BenchmarkSettings bs>
static void locolcw_A(benchmark::State& state) {
    locol_bm_A<bs, dsecs::World>(state);
}

BENCHMARK(locolcw_A<BsUpdate>);
BENCHMARK(locolcw_A<BsInit>);
BENCHMARK(locolcw_A<BsExpand>);//->(BMChurnIter);
BENCHMARK(locolcw_A<BsChurn>);//->(BMChurnIter);
