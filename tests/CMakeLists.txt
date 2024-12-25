

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Catch2)

add_executable(test_hsm4c test_hsm4c.cpp)
target_link_libraries(test_hsm4c PRIVATE hsm4c Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(test_hsm4c)

add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS test_hsm4c)
