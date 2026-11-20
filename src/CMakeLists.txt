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

# Options changing the layout of public types are PUBLIC, so users get the same values.
target_compile_definitions(
  hsm4c
  PUBLIC HSM4C_CONFIG_STATE_NAMES=$<BOOL:${HSM4C_CONFIG_STATE_NAMES}>
         HSM4C_CONFIG_RUN_FN=$<BOOL:${HSM4C_CONFIG_RUN_FN}>
         HSM4C_CONFIG_GUARDS=$<BOOL:${HSM4C_CONFIG_GUARDS}>
         HSM4C_CONFIG_ACTIONS=$<BOOL:${HSM4C_CONFIG_ACTIONS}>
         HSM4C_EVENT_QUEUE_SIZE=${HSM4C_EVENT_QUEUE_SIZE}
         HSM4C_STATE_ID_TYPE=${HSM4C_STATE_ID_TYPE}
  PRIVATE HSM4C_CONFIG_VALIDATE=$<BOOL:${HSM4C_CONFIG_VALIDATE}>
          HSM4C_MAX_TRANSITIONS=${HSM4C_MAX_TRANSITIONS})

hsm4c_target_warnings(hsm4c)
