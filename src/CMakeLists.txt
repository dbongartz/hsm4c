add_library(${PROJECT_NAME} hsm4c.h hsm4c.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
target_compile_features(${PROJECT_NAME} PRIVATE c_std_17)
