# hsm4c

[![CI](https://github.com/dbongartz/hsm4c/actions/workflows/ci.yml/badge.svg)](https://github.com/dbongartz/hsm4c/actions/workflows/ci.yml)
[![Docs](https://github.com/dbongartz/hsm4c/actions/workflows/docs.yml/badge.svg)](https://github.com/dbongartz/hsm4c/actions/workflows/docs.yml)

Hierarchical state machine (statechart) library in C.

The structure (states, pseudo states, transition kinds) follows UML 2. The execution
semantics follow [SCXML](https://www.w3.org/TR/scxml/): event transitions are taken first,
then automatic (event-less) transitions until none is enabled.

- Fully hierarchical states with entry/exit/run functions
- Transitions with guard and action: external, local and internal (targetless)
- Automatic (event-less) transitions
- Shallow and deep history, choice pseudo states
- Table-based, allocation-free definition of states and transitions

See the [API documentation](https://dbongartz.github.io/hsm4c/) and
[`examples/hsm4c_demo.c`](examples/hsm4c_demo.c).

## Usage

Requires a C99 compiler and CMake 3.28 or newer.

With [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html):

```cmake
include(FetchContent)
FetchContent_Declare(
  hsm4c
  GIT_REPOSITORY https://github.com/dbongartz/hsm4c.git
  GIT_TAG v0.2.0)
FetchContent_MakeAvailable(hsm4c)

target_link_libraries(my_app PRIVATE hsm4c::hsm4c)
```

Or with an installed package:

```cmake
find_package(hsm4c 0.2 REQUIRED)
target_link_libraries(my_app PRIVATE hsm4c::hsm4c)
```

```c
#include <hsm4c/hsm4c.h>
```

Tests, examples and install rules are only enabled when hsm4c is the top-level project.

| Option                 | Default        | Description                             |
|------------------------|----------------|-----------------------------------------|
| `HSM4C_BUILD_TESTS`    | top-level only | Build the tests (downloads doctest)     |
| `HSM4C_BUILD_EXAMPLES` | top-level only | Build the examples                      |
| `HSM4C_BUILD_DOCS`     | `OFF`          | Build the API documentation (Doxygen)   |
| `HSM4C_INSTALL`        | top-level only | Generate install rules and CMake package |

`BUILD_SHARED_LIBS` selects a static or shared library.

## Development

The project uses [CMake presets](CMakePresets.json) with Ninja:

```sh
cmake --workflow --preset ci-gcc     # or ci-clang: configure, build, test
cmake --workflow --preset sanitize   # ASan + UBSan
cmake --workflow --preset coverage   # then: gcovr --root . --filter src/ build/coverage
cmake --preset docs && cmake --build --preset docs
```

For development without warnings as errors use the `gcc` or `clang` presets.

Linting, as in CI (clang 18):

```sh
git ls-files '*.c' '*.h' '*.cpp' | xargs clang-format-18 --dry-run --Werror
cmake --preset lint && run-clang-tidy-18 -quiet -p build/lint "$PWD/(src|test|examples)/"
```

The `lint` preset writes `build/lint/compile_commands.json`, which can also be used by clangd.

### Layout

```
src/hsm4c/   library (hsm4c.h is the public header)
test/        doctest unit tests and test statecharts
examples/    examples, also buildable against an installed package
docs/        Doxygen setup
cmake/       CMake helpers and package config
```

### Code coverage

The CI coverage job shows a coverage summary on the workflow run page, a per-file report in
the job log, and uploads a line-by-line HTML report as the `coverage-report` artifact.

## Releasing

Bump `VERSION` in `CMakeLists.txt`, then push a matching `v<version>` tag. CI runs and a
GitHub release with the source archive is created.

## License

MIT, see [LICENSE](LICENSE).
