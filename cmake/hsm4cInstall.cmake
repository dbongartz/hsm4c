include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(HSM4C_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/hsm4c"
    CACHE STRING "Install directory of the hsm4c CMake package files")

install(
  TARGETS hsm4c
  EXPORT hsm4cTargets
  FILE_SET HEADERS)

install(
  EXPORT hsm4cTargets
  NAMESPACE hsm4c::
  DESTINATION "${HSM4C_INSTALL_CMAKEDIR}")

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/hsm4cConfig.cmake.in" "${PROJECT_BINARY_DIR}/hsm4cConfig.cmake"
  INSTALL_DESTINATION "${HSM4C_INSTALL_CMAKEDIR}")

# 0.x: minor versions may break the API
write_basic_package_version_file("${PROJECT_BINARY_DIR}/hsm4cConfigVersion.cmake"
                                 COMPATIBILITY SameMinorVersion)

install(FILES "${PROJECT_BINARY_DIR}/hsm4cConfig.cmake"
              "${PROJECT_BINARY_DIR}/hsm4cConfigVersion.cmake"
        DESTINATION "${HSM4C_INSTALL_CMAKEDIR}")
