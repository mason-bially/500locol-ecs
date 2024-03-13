This repository contains my attempt to write a 500 lines of code or less example of an Entity Component System (ECS) using post-modern C++ (`c++23`). And then explain it in a format similar to the [The Architecture of Open Source Applications](http://aosabook.org/en/index.html) book by a similar name, since I find that format as a useful means of communication to compare myself against.

The actual text can be [found here](manuscript.md), and the [library file `dsecs.hpp` here](dsecs.hpp).

### Notes

The code is 500 lines (or less) as counted by [CLOC](https://github.com/AlDanial/cloc) which does not count blank lines and comments as code.

To run the example use `bazel run example`, to run tests (and display breakages) use `bazel test --test_output=errors //...`.

### Index

I've organized the source code in this repository by chapter so that you can see the code at each stage of it's development.

- `03_trinity` the introduction of the core trinity.
- `0e_ergonomics1` the first round of ergonomic improvements.
- `9z_zero` the final "zero point" of our optimization benchmarks.

Below are the other directories:

- `benchmark` the code for our benchmarking suite
- `compat` compatability headers for dealing with version and implementation differences.
