project(rplidar_sdk)

cmake_minimum_required(VERSION 3.18)

if (NOT UNIX)
    message(FATAL_ERROR "Only support Linux, sorry.")
endif()

file(GLOB SRC_FILES
    sdk/src/*.cpp 
    sdk/src/hal/*.cpp 
    sdk/src/arch/linux/*.cpp)

file(GLOB INCLUDE_FILES
    sdk/include/*.h
    sdk/src/*.h 
    sdk/src/*.hpp 
    sdk/src/hal/*.h 
    sdk/src/hal/*.hpp
    sdk/src/arch/linux/*.h
    sdk/src/arch/linux/*.hpp)

add_library(rplidar_sdk STATIC 
    ${SRC_FILES}
    ${INCLUDE_FILES}
)

target_include_directories(rplidar_sdk
    PUBLIC
        sdk/include
    PRIVATE
        sdk/src
)

set_target_properties(rplidar_sdk PROPERTIES
    PUBLIC_HEADER $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

####################################################
# Exports
####################################################

# This section is a boiler plate to ensure the targets are exported for use in other cmake projects via find_package
# See https://cmake.org/cmake/help/latest/module/CMakePackageConfigHelpers.html for more info on what's going on here

include(CMakePackageConfigHelpers)

# Create a *Config.cmake file using the *.cmake.in file
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/rplidar_sdk_Config.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake
                              INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
)