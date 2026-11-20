/**
 * Property test: runs the fuzz harness (fuzz/fuzz_chart.c) on random inputs, so every test run
 * checks the invariants on thousands of random charts. CI also runs it under libFuzzer.
 */

#include <doctest/doctest.h>

#include "fuzz_chart.h"

#include <cstdint>
#include <random>
#include <vector>

TEST_CASE("random charts keep the invariants") {
  constexpr int runs = 20000;
  std::mt19937 rng(12345); // Fixed seed: reproducible
  std::uniform_int_distribution<int> byte(0, 255);
  std::uniform_int_distribution<std::size_t> length(1, 400);

  int valid = 0;
  for (int i = 0; i < runs; ++i) {
    std::vector<std::uint8_t> input(length(rng));
    for (auto &b : input) {
      b = static_cast<std::uint8_t>(byte(rng));
    }
    fuzz_result const result = fuzz_run(input.data(), input.size());
    REQUIRE_MESSAGE(result != FUZZ_FAILED, "invariant violated for input " << i);
    valid += result == FUZZ_OK ? 1 : 0;
  }
  // The generator must produce enough valid charts to be meaningful
  MESSAGE(valid << " of " << runs << " random charts were valid");
  CHECK(valid > runs / 4);
}
