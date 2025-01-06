file(GLOB EXAMPLE_SOURCES "*.c")

add_custom_target(examples ALL)

foreach(SOURCE_FILE ${EXAMPLE_SOURCES})
    get_filename_component(EXECUTABLE_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(example_${EXECUTABLE_NAME} ${SOURCE_FILE})
    target_link_libraries(example_${EXECUTABLE_NAME} PRIVATE ${PROJECT_NAME})
    target_compile_features(example_${EXECUTABLE_NAME} PRIVATE c_std_17)
    add_dependencies(examples example_${EXECUTABLE_NAME})
endforeach()
