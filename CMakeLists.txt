cmake_minimum_required(VERSION 3.28.0)
project(hsm4c
    VERSION 0.2.0
    DESCRIPTION "Hierarchical State Machines for C"
    LANGUAGES C CXX
)

add_subdirectory(src)
add_subdirectory(examples EXCLUDE_FROM_ALL)
add_subdirectory(tests EXCLUDE_FROM_ALL)
