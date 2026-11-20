# Plain C test that works with any configuration
add_executable(hsm4c_config_test test_config.c)
target_link_libraries(hsm4c_config_test PRIVATE hsm4c::hsm4c)
set_target_properties(hsm4c_config_test PROPERTIES C_EXTENSIONS OFF)
hsm4c_target_warnings(hsm4c_config_test)
add_test(NAME config COMMAND hsm4c_config_test)

# The unit tests use all features
if(NOT (HSM4C_CONFIG_STATE_NAMES AND HSM4C_CONFIG_RUN_FN AND HSM4C_CONFIG_GUARDS
        AND HSM4C_CONFIG_ACTIONS AND HSM4C_CONFIG_VALIDATE))
  message(STATUS "hsm4c: unit tests need all HSM4C_CONFIG_* options, only the config test is built")
  return()
endif()

include(FetchContent)

# Uses an installed doctest if available, downloads it otherwise.
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG 2d0a9359a60c51affe2a9bebb1be1dca47868151 # v2.5.3
  GIT_SHALLOW TRUE
  SYSTEM
  FIND_PACKAGE_ARGS 2.5.3)
set(DOCTEST_NO_INSTALL ON)
FetchContent_MakeAvailable(doctest)

add_executable(hsm4c_tests test_hsm4c.cpp test_chart.c test_scxml.cpp scxml_charts.c
                           test_fuzz.cpp ${PROJECT_SOURCE_DIR}/fuzz/fuzz_chart.c)
target_include_directories(hsm4c_tests PRIVATE ${PROJECT_SOURCE_DIR}/fuzz)
target_link_libraries(hsm4c_tests PRIVATE hsm4c::hsm4c doctest::doctest)
target_compile_features(hsm4c_tests PRIVATE cxx_std_17)
set_target_properties(hsm4c_tests PROPERTIES C_EXTENSIONS OFF CXX_EXTENSIONS OFF)
hsm4c_target_warnings(hsm4c_tests)

if(doctest_FOUND)
  include(doctest)
else()
  include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
endif()
doctest_discover_tests(hsm4c_tests)
