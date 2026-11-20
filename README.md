# hsm4c

[![CI](https://github.com/dbongartz/hsm4c/actions/workflows/ci.yml/badge.svg)](https://github.com/dbongartz/hsm4c/actions/workflows/ci.yml)
[![Docs](https://github.com/dbongartz/hsm4c/actions/workflows/docs.yml/badge.svg)](https://github.com/dbongartz/hsm4c/actions/workflows/docs.yml)

Hierarchical state machine (statechart) library in C.

The structure (states, pseudo states, transition kinds) follows UML 2. The execution
semantics follow [SCXML](https://www.w3.org/TR/scxml/): event transitions are taken first,
then automatic (event-less) transitions and internal events until none is left.

- Fully hierarchical states with entry, exit and run functions
- Transitions with guard and action: external, local and internal (targetless)
- Automatic (event-less) transitions and internal events raised from callbacks
- Shallow and deep history, choice pseudo states
- Events with user data, a user context for all callbacks
- Constant charts that can live in flash, no dynamic memory, no recursion
- Chart validation, bounded processing, result codes instead of undefined behaviour

See the [API documentation](https://dbongartz.github.io/hsm4c/) and
[`examples/hsm4c_demo.c`](examples/hsm4c_demo.c).

## Example

```c
#include <hsm4c/hsm4c.h>

enum { ROOT, OFF, ON, NUM_STATES }; /* State 0 is the root */
enum { EV_TOGGLE = 1 };             /* Events are > 0 */

static void on_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  int *switched_on = hsm4c_ctx(sm);
  ++*switched_on;
}

static hsm4c_transition_t const off_transitions[] = {
    {.event = EV_TOGGLE, .to = ON},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const on_transitions[] = {
    {.event = EV_TOGGLE, .to = OFF},
    HSM4C_TRANSITIONS_END,
};

/* .parent defaults to ROOT */
static hsm4c_state_t const states[NUM_STATES] = {
    [ROOT] = {.initial = OFF},
    [OFF] = {.transitions = off_transitions},
    [ON] = {.entry_fn = on_entry, .transitions = on_transitions},
};
static hsm4c_chart_t const chart = HSM4C_CHART(states);

int main(void) {
  int switched_on = 0;
  hsm4c_t sm;
  hsm4c_state_id_t active[NUM_STATES]; /* Runtime data, one entry per state */

  if (hsm4c_init(&sm, &chart, active, &switched_on) != HSM4C_OK) {
    return 1;
  }
  hsm4c_run(&sm, EV_TOGGLE, NULL); /* -> ON */
  return hsm4c_current(&sm) == ON && switched_on == 1 ? 0 : 1;
}
```

## Usage

Requires a C99 compiler and CMake 3.28 or newer.

With [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html):

```cmake
include(FetchContent)
FetchContent_Declare(
  hsm4c
  GIT_REPOSITORY https://github.com/dbongartz/hsm4c.git
  GIT_TAG v0.3.0)
FetchContent_MakeAvailable(hsm4c)

target_link_libraries(my_app PRIVATE hsm4c::hsm4c)
```

Or with an installed package:

```cmake
find_package(hsm4c 0.3 REQUIRED)
target_link_libraries(my_app PRIVATE hsm4c::hsm4c)
```

Tests, examples and install rules are only enabled when hsm4c is the top-level project.

| Option                 | Default        | Description                              |
|------------------------|----------------|------------------------------------------|
| `HSM4C_BUILD_TESTS`    | top-level only | Build the tests (downloads doctest)      |
| `HSM4C_BUILD_EXAMPLES` | top-level only | Build the examples                       |
| `HSM4C_BUILD_DOCS`     | `OFF`          | Build the API documentation (Doxygen)    |
| `HSM4C_BUILD_FUZZERS`  | `OFF`          | Build the libFuzzer target (Clang)       |
| `HSM4C_INSTALL`        | top-level only | Generate install rules and CMake package |

`BUILD_SHARED_LIBS` selects a static or shared library.

### Configuration

These CMake cache variables configure the library. Those that change the layout of public types
are passed on to everything linking `hsm4c::hsm4c`. Without CMake, define the macros of the
same name for the library and all code including `hsm4c.h`.

| Variable                   | Default    | Description                                        |
|----------------------------|------------|----------------------------------------------------|
| `HSM4C_CONFIG_STATE_NAMES` | `ON`       | States have a `name` field                         |
| `HSM4C_CONFIG_RUN_FN`      | `ON`       | States have a run function                         |
| `HSM4C_CONFIG_GUARDS`      | `ON`       | Transitions have a guard                           |
| `HSM4C_CONFIG_ACTIONS`     | `ON`       | Transitions have an action                         |
| `HSM4C_CONFIG_VALIDATE`    | `ON`       | `hsm4c_init()` validates the chart                 |
| `HSM4C_MAX_TRANSITIONS`    | `64`       | Longest chain of transitions in one call           |
| `HSM4C_EVENT_QUEUE_SIZE`   | `8`        | Capacity of the internal event queue (1 to 255)    |
| `HSM4C_STATE_ID_TYPE`      | `uint16_t` | State ID type, limits the number of states         |

Define `HSM4C_ASSERT(expr)` when compiling the library to replace `assert` for programming
errors such as NULL arguments.

### Footprint

The library has no static RAM and never allocates. A machine (`hsm4c_t`) holds the internal
event queue, and the runtime array has one state ID per state. Measured by the CI `embedded`
job with `arm-none-eabi-gcc -Os` (GCC 13) on Cortex-M0+:

| Configuration | Code    | Worst-case stack of `hsm4c_run()` |
|---------------|---------|-----------------------------------|
| All features  | 1426 B  | 176 B, plus callbacks             |
| Minimal       | 1064 B  | 160 B, plus callbacks             |

The worst-case stack is computed from GCC's call graph: nothing recurses, so it is bounded.

## Migrating from 0.2

0.3 changes the whole API:

| 0.2                                       | 0.3                                                       |
|-------------------------------------------|-----------------------------------------------------------|
| `State` + `StateConfig`, linked by pointers | Constant `hsm4c_state_t` array indexed by state ID, root is ID 0, wrapped in `hsm4c_chart_t` |
| `sc_map_stateconfig_to_states()`, `sc_reset_state()` | Removed: `hsm4c_init()` takes the chart and a runtime array and clears all history |
| `sc_init(root)`, `sc_run(root, event)` return the leaf | `hsm4c_init(&sm, &chart, active, ctx)`, `hsm4c_run(&sm, event, data)` return `hsm4c_result_t`; the leaf is `hsm4c_current(&sm)` |
| Root table with `from` field, or per-state tables | Per-state tables only, rows have no source |
| Row order `{from, to, event, transition_fn, guard_fn, type}` | `{event, to, guard_fn, action_fn, type}`; designated initializers recommended |
| `SC_TYPE_ROOT`                            | Removed, the root is state 0                              |
| Callbacks get `State const *`             | Callbacks get the machine (`hsm4c_t *`), a state ID and/or the event (`hsm4c_event_t const *`); `hsm4c_ctx(sm)` gives the user context |
| Run functions return `State *`            | Return a state ID, `HSM4C_NO_STATE` for no change         |
| Local transition "same -> same" was internal | Behaves as external (SCXML); use `to = HSM4C_NO_STATE` for an internal transition |
| `sc_*`, `SC_*`, `State`, `Transition`, ... | `hsm4c_*`, `HSM4C_*` names throughout                    |

New in 0.3: internal events (`hsm4c_raise()`), chart validation (`hsm4c_validate()`),
`hsm4c_is_active()`, the transition limit and compile-time configuration.

## Development

The project uses [CMake presets](CMakePresets.json) with Ninja:

```sh
cmake --workflow --preset ci-gcc     # or ci-clang: configure, build, test
cmake --workflow --preset sanitize   # ASan + UBSan
cmake --workflow --preset minimal    # all optional features off
cmake --workflow --preset coverage   # then: gcovr --root . --filter src/ build/coverage
cmake --preset docs && cmake --build --preset docs
```

For development without warnings as errors use the `gcc` or `clang` presets.

Fuzzing (Clang with libFuzzer) and embedded builds (`arm-none-eabi-gcc`):

```sh
cmake --preset fuzz && cmake --build --preset fuzz
build/fuzz/fuzz/hsm4c_fuzz -max_total_time=60

cmake --preset cortex-m0plus && cmake --build --preset cortex-m0plus
tools/embedded_report.py build/cortex-m0plus
```

Linting, as in CI (clang 18):

```sh
git ls-files '*.c' '*.h' '*.cpp' | xargs clang-format-18 --dry-run --Werror
cmake --preset lint && run-clang-tidy-18 -quiet -p build/lint "$PWD/(src|test|examples|fuzz)/"
```

The `lint` preset writes `build/lint/compile_commands.json`, which can also be used by clangd.

### Tests

- Unit tests (`test/test_hsm4c.cpp`) for the two test charts, validation and all error paths.
- SCXML conformance (`test/test_scxml.cpp`): charts ported from the
  [W3C SCXML test suite](https://www.w3.org/Voice/2013/scxml-irp/), see `test/scxml_charts.c`.
- Fuzzing: `fuzz/fuzz_chart.c` builds random charts and checks invariants. A property test runs
  it on 20000 random inputs in every test run, and CI runs it under libFuzzer.
- `test/test_config.c` works with every configuration, see the `minimal` preset.

CI fails below 100% line and branch coverage of the library (assert branches excluded). The
coverage job shows a summary on the workflow run page, a per-file report in the job log, and
uploads a line-by-line HTML report as the `coverage-report` artifact.

### Layout

```
src/hsm4c/   library (hsm4c.h is the public header)
test/        unit tests, test statecharts, SCXML conformance tests
fuzz/        fuzz harness and libFuzzer target
examples/    examples, also buildable against an installed package
docs/        Doxygen setup
cmake/       CMake helpers, package config, ARM toolchain
tools/       embedded size and stack report
```

## Releasing

Bump `VERSION` in `CMakeLists.txt`, then push a matching `v<version>` tag. CI runs and a
GitHub release with the source archive is created.

## License

MIT, see [LICENSE](LICENSE).
