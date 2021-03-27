#pragma once

#include <cstdint>

namespace AWEngine::Packet
{
    AWE_ENUM_FLAGS(PacketFlags, uint8_t)
    {
        Compressed  = 1u << 0u,
        Unused_Bit1 = 1u << 1u,
        Unused_Bit2 = 1u << 2u,
        Unused_Bit3 = 1u << 3u,
        Unused_Bit4 = 1u << 4u,
        Unused_Bit5 = 1u << 5u,
        Unused_Bit6 = 1u << 6u,
        Unused_Bit7 = 1u << 7u,
    };
}
