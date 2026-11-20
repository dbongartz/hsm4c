/* libFuzzer target, see fuzz_chart.h. Run: build/fuzz/fuzz/hsm4c_fuzz -max_total_time=60 */

#include "fuzz_chart.h"

#include <stdlib.h>

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size);

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
  if (fuzz_run(data, size) == FUZZ_FAILED) {
    abort();
  }
  return 0;
}
