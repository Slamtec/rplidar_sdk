cmake_minimum_required(VERSION 3.18)
project(rplidar_sdk)

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sdk/src>
        $<INSTALL_INTERFACE:include>
        # $<INSTALL_INTERFACE:sdk/src>
)

file(GLOB PUBLIC_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/*.h")
install(FILES ${PUBLIC_HEADER_FILES}
    DESTINATION include
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sdk/src/hal/types.h"
    DESTINATION include/hal
)


install(TARGETS rplidar_sdk
    EXPORT ${PROJECT_NAME}-targets
)

####################################################
# Exports
####################################################

# This section is a boiler plate to ensure the targets are exported for use in other cmake projects via find_package
# See https://cmake.org/cmake/help/latest/module/CMakePackageConfigHelpers.html for more info on what's going on here

include(CMakePackageConfigHelpers)

# Create a *Config.cmake file using the *.cmake.in file
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake
                              INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

# Install the *Config.cmake file we've just created
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}/
)

# Install the targets exported by the earlier install commands
# for use by the *Config.cmake generate above
# This file is pulled in via the commands in the *Config.cmake.in template
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}-targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION lib/cmake/${PROJECT_NAME}
)