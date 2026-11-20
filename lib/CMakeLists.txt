add_library(hsm4c hsm4c.c)

set_property(TARGET hsm4c PROPERTY C_STANDARD 17)
target_include_directories(hsm4c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(hsm4c PRIVATE -pedantic)
