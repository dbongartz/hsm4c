add_executable(test_hsm4c test_hsm4c.cpp test_chart.c)
target_link_libraries(test_hsm4c PRIVATE hsm4c)

set_property(TARGET test_hsm4c PROPERTY C_STANDARD 17)
set_property(TARGET test_hsm4c PROPERTY CXX_STANDARD 17)

add_test(NAME test_hsm4c COMMAND test_hsm4c)
