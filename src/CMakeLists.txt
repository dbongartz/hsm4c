add_library(hsm4c)
add_library(hsm4c::hsm4c ALIAS hsm4c)

target_sources(
  hsm4c
  PRIVATE hsm4c/hsm4c.c
  PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" FILES hsm4c/hsm4c.h)

# Public header needs C99 (bool, designated initializers, VLA parameter declarations)
target_compile_features(hsm4c PUBLIC c_std_99)

set_target_properties(
  hsm4c
  PROPERTIES C_EXTENSIONS OFF
             VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

hsm4c_target_warnings(hsm4c)
