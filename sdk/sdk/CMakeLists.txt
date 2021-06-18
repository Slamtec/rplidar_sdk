cmake_minimum_required(VERSION 3.10)


file(GLOB base_sources src/*.cpp src/hal/*.cpp)
file(GLOB base_includes src/*.h src/*.hpp src/hal/*.h src/hal/*.hpp)

# Check if Windows, exclude MinGW
if (WIN32 AND NOT MINGW)
    message(STATUS "Detected Windows target...")
    file(GLOB platform_srcs src/arch/win32/*.hpp src/arch/win32/*.cpp)
    include_directories(src/arch/win32)
elseif (MINGW)
    # Library doesn't build under MinGW, no idea why.
    message(FATAL_ERROR "Mingw is presently not supported.")
elseif (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    message(STATUS "linux target...")
    file(GLOB platform_srcs src/arch/linux/*.hpp src/arch/linux/*.cpp)
    include_directories(src/arch/linux)

elseif (${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    message(STATUS "Apple Mac OS X target...")
    file(GLOB platform_srcs src/arch/macOS/*.hpp src/arch/macOS/*.cpp)
    include_directories(src/arch/macOS)
else ()
    message(FATAL_ERROR "Unknown platform ${CMAKE_SYSTEM_NAME}, build cannot continue.")
endif ()

# glob shared C++ sources
add_library(rplidar_sdk STATIC ${base_sources} ${platform_srcs})

target_include_directories(
        rplidar_sdk INTERFACE include src
)

include_directories(src src/hal include)

install(
        TARGETS rplidar_sdk
        DESTINATION .
)

