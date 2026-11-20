add_executable(hsm4c_demo hsm4c_demo.c)
target_link_libraries(hsm4c_demo PRIVATE hsm4c)

set_property(TARGET hsm4c_demo PROPERTY C_STANDARD 17)
