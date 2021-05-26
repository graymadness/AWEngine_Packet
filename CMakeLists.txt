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
# Configuration
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

target_link_libraries(AWEngine_Packet asio pthread)

if(GCC)
    target_link_libraries(AWEngine_Packet -static-libgcc -static-libstdc++ stdc++fs)
endif()

# JSON Support
if(TARGET nlohmann_json)
    message(NOTICE "nlohmann's JSON library detected")

    set(AWE_PACKET_LIB_JSON=ON)
    target_compile_definitions(AWEngine_Packet AWE_PACKET_LIB_JSON=ON)
    target_link_libraries(AWEngine_Packet nlohmann_json)
endif()

# Protocol Version
if(DEFINED AWE_PACKET_PROTOCOL_VERSION)
    if(AWE_PACKET_PROTOCOL_VERSION MATCHES "^[0-9]+$")
        message(NOTICE "Protocol Version set to ${AWE_PACKET_PROTOCOL_VERSION}")
        target_compile_definitions(AWEngine_Packet AWE_PACKET_PROTOCOL_VERSION=${AWE_PACKET_PROTOCOL_VERSION})
    else()
        message(FATAL_ERROR "Invalid Protocol Version '${AWE_PACKET_PROTOCOL_VERSION}' - only numeric value is allowed")
    endif()
endif()

# Game Name
if(DEFINED AWE_PACKET_GAME_NAME)
    if(AWE_PACKET_GAME_NAME MATCHES "^[A-Z]+$")
        message(NOTICE "Protocol Version set to ${AWE_PACKET_GAME_NAME}")
        target_compile_definitions(AWEngine_Packet AWE_PACKET_GAME_NAME="${AWE_PACKET_GAME_NAME}")
    else()
        message(FATAL_ERROR "Invalid Protocol Game Name '${AWE_PACKET_GAME_NAME}' - use only UPPER CASE letters")
    endif()
endif()

# Minimum / Maximum threads for Packet Server (used for IO Context)
if(DEFINED AWE_PACKET_SERVER_THREAD_MIN)
    add_compile_definitions(AWEngine_Packet AWE_PACKET_SERVER_THREAD_MIN=${AWE_PACKET_SERVER_THREAD_MIN})
else()
    add_compile_definitions(AWEngine_Packet AWE_PACKET_SERVER_THREAD_MIN=1)
endif()
if(DEFINED AWE_PACKET_SERVER_THREAD_MAX)
    add_compile_definitions(AWEngine_Packet AWE_PACKET_SERVER_THREAD_MAX=${AWE_PACKET_SERVER_THREAD_MAX})
else()
    add_compile_definitions(AWEngine_Packet AWE_PACKET_SERVER_THREAD_MAX=1)
endif()

#--------------------------------
# Tests
#--------------------------------

if(BUILD_TESTING)
    include(CTest)
    enable_testing()

    add_subdirectory(tests)
endif()
