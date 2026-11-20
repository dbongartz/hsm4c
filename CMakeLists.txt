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
option(HSM4C_INSTALL "Generate the hsm4c install rules" ${PROJECT_IS_TOP_LEVEL})

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

if(HSM4C_BUILD_DOCS)
  add_subdirectory(docs)
endif()

if(HSM4C_INSTALL)
  include(cmake/hsm4cInstall.cmake)
endif()
