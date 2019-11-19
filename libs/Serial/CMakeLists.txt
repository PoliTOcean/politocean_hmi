cmake_minimum_required(VERSION 3.5)
project(Serial VERSION 1.0.0 LANGUAGES CXX)

add_library(Serial SHARED
        Serial.cpp)

add_library(PolitoceanHmi::Serial ALIAS Serial)

target_include_directories(Serial
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Serial 
                                PolitoceanCommon::logger
                                ${OpenCV_LIBS})

target_compile_features(Serial PRIVATE cxx_auto_type)
target_compile_options(Serial PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

include(GNUInstallDirs)
install(TARGETS Serial
        EXPORT PolitoceanHmiTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
