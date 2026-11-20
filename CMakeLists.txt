cmake_minimum_required(VERSION 3.14)
project(hsm4c VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

add_compile_options(-Wall -Wextra)

include(CTest)

add_subdirectory(lib)
add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
