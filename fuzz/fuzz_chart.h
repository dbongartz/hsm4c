#pragma once
/*
 * Fuzz harness: builds a random chart from input bytes, runs it and checks invariants.
 * Used by the libFuzzer target (fuzz_hsm4c.c) and by a property test in the unit tests.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Result of fuzz_run(). */
enum fuzz_result {
  /** \brief The input does not describe a valid chart, nothing was run. */
  FUZZ_SKIPPED = -1,
  /** \brief The chart ran and all invariants held. */
  FUZZ_OK = 0,
  /** \brief An invariant was violated. */
  FUZZ_FAILED = 1,
};

/**
 * \brief Builds a chart from data, initializes it and processes events taken from data,
 * checking invariants after every call.
 */
enum fuzz_result fuzz_run(uint8_t const *data, size_t size);

#ifdef __cplusplus
}
#endif
