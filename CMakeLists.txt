cmake_minimum_required(VERSION 3.28)

project(
  hsm4c
  VERSION 0.2.0
  DESCRIPTION "Hierarchical state machine (statechart) library in C"
  HOMEPAGE_URL "https://github.com/dbongartz/hsm4c"
  LANGUAGES C)

option(HSM4C_BUILD_TESTS "Build the hsm4c tests" ${PROJECT_IS_TOP_LEVEL})
option(HSM4C_BUILD_EXAMPLES "Build the hsm4c examples" ${PROJECT_IS_TOP_LEVEL})
option(HSM4C_BUILD_DOCS "Build the hsm4c API documentation (requires Doxygen)" OFF)
option(HSM4C_BUILD_FUZZERS "Build the hsm4c libFuzzer target (requires Clang)" OFF)
option(HSM4C_INSTALL "Generate the hsm4c install rules" ${PROJECT_IS_TOP_LEVEL})

# Library configuration, see "Configuration" in src/hsm4c/hsm4c.h
option(HSM4C_CONFIG_STATE_NAMES "States have a name field" ON)
option(HSM4C_CONFIG_RUN_FN "States have a run function" ON)
option(HSM4C_CONFIG_GUARDS "Transitions have a guard" ON)
option(HSM4C_CONFIG_ACTIONS "Transitions have an action" ON)
option(HSM4C_CONFIG_VALIDATE "hsm4c_init() validates the chart" ON)
set(HSM4C_MAX_TRANSITIONS 64 CACHE STRING "Maximum transitions per hsm4c_init()/hsm4c_run()")
set(HSM4C_EVENT_QUEUE_SIZE 8 CACHE STRING "Internal event queue capacity (1 to 255)")
set(HSM4C_STATE_ID_TYPE uint16_t CACHE STRING "Unsigned integer type of state IDs")

include(cmake/hsm4cWarnings.cmake)

add_subdirectory(src)

if(HSM4C_BUILD_TESTS)
  enable_language(CXX)
  enable_testing()
  add_subdirectory(test)
endif()

if(HSM4C_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(HSM4C_BUILD_FUZZERS)
  add_subdirectory(fuzz)
endif()

if(HSM4C_BUILD_DOCS)
  add_subdirectory(docs)
endif()

if(HSM4C_INSTALL)
  include(cmake/hsm4cInstall.cmake)
endif()
