cmake_minimum_required(VERSION 3.5)
project(Joystick VERSION 1.0.0 LANGUAGES CXX)

add_library(Joystick SHARED
        Joystick.cpp)

add_library(PolitoceanHmi::Joystick ALIAS Joystick)

target_include_directories(Joystick
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Joystick -lpthread PolitoceanCommon::mqttLogger)

target_compile_features(Joystick PRIVATE cxx_auto_type)
target_compile_options(Joystick PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

include(GNUInstallDirs)
install(TARGETS Joystick
        EXPORT PolitoceanHmiTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
