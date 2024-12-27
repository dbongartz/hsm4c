

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(
    trompeloeil
    GIT_REPOSITORY https://github.com/rollbear/trompeloeil.git
    GIT_TAG v49
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Catch2 trompeloeil)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

add_executable(test_hsm4c test_hsm4c.cpp)
target_link_libraries(test_hsm4c PRIVATE ${PROJECT_NAME} Catch2::Catch2WithMain trompeloeil)
target_compile_features(test_hsm4c PRIVATE cxx_std_20 c_std_17)
catch_discover_tests(test_hsm4c)

add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS test_hsm4c test_hsm4c_conv)
