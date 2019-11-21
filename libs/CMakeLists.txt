cmake_minimum_required(VERSION 3.10)
project(PolitoceanHmi)

# Add here all libraries' directories
#add_subdirectory(name_of_directory)
add_subdirectory(Joystick)
add_subdirectory(Serial)

# Install and export (do not touch this part)
install(
  EXPORT PolitoceanHmiTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PolitoceanHmi
  NAMESPACE PolitoceanHmi::
  FILE PolitoceanHmiTargets.cmake
  )

include(CMakePackageConfigHelpers)
configure_package_config_file( 
  "Config.cmake.in" 
  "PolitoceanHmiConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PolitoceanHmi
  PATH_VARS
    CMAKE_INSTALL_LIBDIR
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/PolitoceanHmiConfigVersion.cmake
  VERSION 1.0.0
  COMPATIBILITY SameMajorVersion
  )

### Install Config and ConfigVersion files
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/PolitoceanHmiConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/PolitoceanHmiConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PolitoceanHmi"
  )
