
include_directories(lib)
include_directories(/opt/ruby/3.0.3/lib/ruby/gems/3.0.0/gems/ceedling-0.31.1/vendor/unity/src/)

add_executable(hsm4c_demo hsm4c_demo.c)
target_link_libraries(hsm4c_demo PUBLIC hsm4c)

set_property(TARGET hsm4c_demo PROPERTY C_STANDARD 17)

target_include_directories(hsm4c_demo PUBLIC "${PROJECT_SOURCE_DIR}/lib")
