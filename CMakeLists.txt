cmake_minimum_required(VERSION 3.5)
project(PolitoceanHmi)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
	In-source builds are not allowed.
	Please, create a separate directory for build files (e.g. build/).
	Delete CMakeCache.txt, make a separate dir and try again.
	")
endif()

include_directories(include)

# Add politocean common libs
add_subdirectory(politocean_common)
include_directories(politocean_common/include)

# Add project internal libs
add_subdirectory(libs)

# Add executable
add_executable(PolitoceanJoystick src/JoystickPublisher.cpp)
add_executable(PolitoceanCommands src/CommandParser.cpp)
add_executable(PolitoceanMouse src/Mouse.cpp)
add_executable(PolitoceanPhMeter src/PhMeter.cpp)
    
# Linking the libraries
target_link_libraries(PolitoceanJoystick -lpthread
    PolitoceanHmi::Joystick
    PolitoceanCommon::mqttLogger
    PolitoceanCommon::MqttClient
    PolitoceanCommon::Component)
    
target_link_libraries(PolitoceanCommands -lpthread
        PolitoceanCommon::mqttLogger
        PolitoceanCommon::MqttClient
        PolitoceanCommon::Component)

target_link_libraries(PolitoceanMouse -lX11
        PolitoceanCommon::MqttClient
        PolitoceanCommon::mqttLogger)

target_link_libraries(PolitoceanPhMeter
        PolitoceanHmi::Serial)

IF( BUILD_TESTS )
    #test section
    find_package(Catch2 REQUIRED)
    add_executable(MqttTest test/MqttTest.cpp)
    target_link_libraries(MqttTest Catch2::Catch2
        PolitoceanCommon::MqttClient
        PolitoceanCommon::mqttLogger)
endif()


# install
set(CMAKE_INSTALL_PREFIX:PATH /usr)
include(GNUInstallDirs)

install ( TARGETS ${PolitoceanHmi_LIBS} ${PolitoceanCommon_LIBS} 
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})