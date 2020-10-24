cmake_minimum_required(VERSION 3.16)
project(AWEngine_Packet C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

cmake_policy(SET CMP0077 NEW) # set() overrides option()

#--------------------------------
# Debug Options
#--------------------------------

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>") # Define DEBUG in Debug builds

#--------------------------------
# Compiler configuration
#--------------------------------

if(MINGW)
    ADD_COMPILE_OPTIONS(-lstdc++fs)
    set(WIN32 ON)
endif()

#--------------------------------
# External libraries
#--------------------------------

add_subdirectory(lib)

#--------------------------------
# Source
#--------------------------------

file(GLOB_RECURSE AWE_Packet_SOURCE src/*.hpp src/*.cpp src/*.h src/*.c)

add_library(AWEngine_Packet STATIC ${AWE_Packet_SOURCE})
target_include_directories(AWEngine_Packet PUBLIC src/)

target_link_libraries(AWEngine_Packet -static-libgcc -static-libstdc++ stdc++fs)
target_link_libraries(AWEngine_Packet Asio)

#--------------------------------
# Tests
#--------------------------------

if(BUILD_TESTING)
    include(CTest)
    enable_testing()

    add_subdirectory(tests)
endif()
