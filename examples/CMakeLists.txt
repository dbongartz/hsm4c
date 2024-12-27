add_executable(example_nontrivial nontrivial.c)
target_link_libraries(example_nontrivial PRIVATE ${PROJECT_NAME})
target_compile_features(example_nontrivial PRIVATE c_std_17)
