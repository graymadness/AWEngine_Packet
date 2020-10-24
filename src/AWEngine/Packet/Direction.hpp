#pragma once

#include <cstdint>

namespace AWEngine::Packet
{
    enum class Direction : uint8_t
    {
        /// From server to client
        ToClient = 0,

        /// From client to server
        ToServer = 1
    };
}
