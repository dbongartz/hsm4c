
include_directories(lib)

add_executable(hsm4c_test hsm4c_test.c)
target_link_libraries(hsm4c_test PUBLIC hsm4c)

set_property(TARGET hsm4c_test PROPERTY C_STANDARD 99)

target_include_directories(hsm4c_test PUBLIC "${PROJECT_SOURCE_DIR}/lib")