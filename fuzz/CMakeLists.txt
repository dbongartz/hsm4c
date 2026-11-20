# libFuzzer target. Needs Clang; the property test in test/ runs the same harness without it.
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "HSM4C_BUILD_FUZZERS needs Clang (libFuzzer)")
endif()

add_executable(hsm4c_fuzz fuzz_hsm4c.c fuzz_chart.c)
target_link_libraries(hsm4c_fuzz PRIVATE hsm4c::hsm4c)
set_target_properties(hsm4c_fuzz PROPERTIES C_EXTENSIONS OFF)
hsm4c_target_warnings(hsm4c_fuzz)

# Instrument the library too, so the fuzzer sees its coverage
set(fuzz_flags -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all)
target_compile_options(hsm4c_fuzz PRIVATE ${fuzz_flags})
target_compile_options(hsm4c PRIVATE -fsanitize=fuzzer-no-link,address,undefined
                                     -fno-sanitize-recover=all)
target_link_options(hsm4c_fuzz PRIVATE ${fuzz_flags})
