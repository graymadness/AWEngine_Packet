cmake_minimum_required(VERSION 3.16)
project(AWEngine_Packet C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # set() overrides option()

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>") # Define DEBUG in Debug builds

#--------------------------------
# Options
#--------------------------------

option(AWE_PACKET_LIB_JSON "Allows nlohmann's JSON for ServerInfo packet" OFF)

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

target_precompile_headers(AWEngine_Packet PUBLIC
    src/AWEngine/Packet/Util/Core_Packet.hpp
)

if(WIN32)
    target_link_libraries(AWEngine_Packet ws2_32)
endif()

# JSON Support
if(TARGET nlohmann_json)
    if(AWE_PACKET_LIB_JSON)
        message(NOTICE "Will use nlohmann's JSON library")

        target_compile_definitions(AWEngine_Packet PUBLIC AWE_PACKET_LIB_JSON=ON)
        target_link_libraries(AWEngine_Packet nlohmann_json)
    else() # NOT AWE_PACKET_LIB_JSON
        message(WARNING "nlohmann's JSON library detected but not used, consider setting AWE_PACKET_LIB_JSON=ON")
    endif()
else() # NOT TARGET nlohmann_json
    if(AWE_PACKET_LIB_JSON)
        message(WARNING "nlohmann's JSON library not found but requested, either add target `nlohmann_json` or set AWE_PACKET_LIB_JSON=OFF")
    endif()
endif()

# Protocol Version
if(DEFINED AWE_PACKET_PROTOCOL_VERSION)
    if(AWE_PACKET_PROTOCOL_VERSION NOT MATCHES "^[0-9]+$")
        message(FATAL_ERROR "Invalid Protocol Version '${AWE_PACKET_PROTOCOL_VERSION}' - can only be numeric value")
    endif()
    message(NOTICE "Protocol Version set to ${AWE_PACKET_PROTOCOL_VERSION}")
    target_compile_definitions(AWEngine_Packet PUBLIC AWE_PACKET_PROTOCOL_VERSION=${AWE_PACKET_PROTOCOL_VERSION})
endif()

# Game Name
if(DEFINED AWE_PACKET_GAME_NAME)
    if(AWE_PACKET_GAME_NAME NOT MATCHES "^[A-Z_]+$")
        message(WARNING "Protocol Game Name '${AWE_PACKET_GAME_NAME}' should only contain upper-case A-Z letters and underscore")
    endif()

    message(NOTICE "Default Protocol Game Name set to ${AWE_PACKET_GAME_NAME}")
    target_compile_definitions(AWEngine_Packet PUBLIC AWE_PACKET_GAME_NAME="${AWE_PACKET_GAME_NAME}")
endif()

#--------------------------------
# Tests
#--------------------------------

if(BUILD_TESTING)
    enable_testing()

    add_subdirectory(tests)
endif()
