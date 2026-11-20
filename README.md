# hsm4c

[![Build Linux](https://github.com/dbongartz/hsm4c/actions/workflows/build_linux.yml/badge.svg)](https://github.com/dbongartz/hsm4c/actions/workflows/build_linux.yml)

Hierarchical state machine (UML statechart) library in C.

## Build and test

Requires CMake and a C17/C++17 compiler. Tests use the vendored [doctest](https://github.com/doctest/doctest) header (`test/doctest.h`).

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
