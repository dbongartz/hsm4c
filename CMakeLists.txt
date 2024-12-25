cmake_minimum_required(VERSION 3.28.0)
project(hsm4c
    VERSION 0.2.0
    DESCRIPTION "Hierarchical State Machine for C"
    LANGUAGES C CXX
)

# include(CTest)
# enable_testing()
add_subdirectory(src)
add_subdirectory(examples EXCLUDE_FROM_ALL)
add_subdirectory(tests EXCLUDE_FROM_ALL)

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
