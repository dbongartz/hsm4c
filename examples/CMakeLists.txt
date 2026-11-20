# Can be built as part of hsm4c, or standalone against an installed hsm4c:
#   cmake -S examples -B build-examples -DCMAKE_PREFIX_PATH=<hsm4c install prefix>
cmake_minimum_required(VERSION 3.28)
project(hsm4c_examples LANGUAGES C)

if(NOT TARGET hsm4c::hsm4c)
  find_package(hsm4c 0.1 REQUIRED)
endif()

add_executable(hsm4c_demo hsm4c_demo.c)
target_link_libraries(hsm4c_demo PRIVATE hsm4c::hsm4c)
set_target_properties(hsm4c_demo PROPERTIES C_EXTENSIONS OFF)
target_compile_options(hsm4c_demo PRIVATE $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>)
