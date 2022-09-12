This repository contains my attempt to write a 500 lines of code or less example of an Entity Component System (ECS). And then explain it in a format similar to the [The Architecture of Open Source Applications](http://aosabook.org/en/index.html) book by a similar name, since I find that format as a useful means of communication to compare myself against.

The actual text can be [found here](manuscript.md).

### Notes

The code is 500 lines (or less) as counted by [CLOC](https://github.com/AlDanial/cloc) which does not skip blank lines and comments.

To run the example use `bazel run example`, to run tests (and display breakages) use `bazel test --test_output=errors //...`.