add_library(hsm4c hsm4c.h hsm4c.c)

target_include_directories(hsm4c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(hsm4c PRIVATE -Wall -Wextra -pedantic)
target_compile_features(hsm4c PRIVATE c_std_17)
