cmake_minimum_required(VERSION 3.0.0)
project(hsm4c VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

add_subdirectory(src)
add_subdirectory(lib)

add_compile_options(-Og -g)
add_compile_options(-Wall -Wextra -pedantic)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
