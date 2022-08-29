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
    sdk/src/*.h 
    sdk/src/*.hpp 
    sdk/src/hal/*.h 
    sdk/src/hal/*.hpp
    sdk/src/arch/linux/*.h
    sdk/src/arch/linux/*.hpp)

add_library(rplidar_sdk 
    STATIC 
        ${SRC_FILES}
        ${INCLUDE_FILES}
)

target_include_directories(rplidar_sdk
    PUBLIC
        sdk/include
        sdk/src
)