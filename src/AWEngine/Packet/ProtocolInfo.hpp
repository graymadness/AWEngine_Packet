#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include "PacketBuffer.hpp"

namespace AWEngine::Packet
{
    template<typename TPacketEnum>
    class IPacket;

    typedef uint8_t PacketID_t;
    static const std::size_t PacketID_Count = static_cast<std::size_t>(std::numeric_limits<PacketID_t>::max()) + 1;

    typedef uint32_t ProtocolVersion_t;

    typedef uint32_t ProtocolVersion_t;

    static const std::size_t GameName_Length = 8;
    typedef std::array<char, GameName_Length> GameName_t;

    AWE_ENUM_FLAGS(PacketFlags, uint8_t)
    {
        /// Data in the packet are compressed by ________
        Compressed  = 1u << 0u,
        Unused_Bit1 = 1u << 1u,
        Unused_Bit2 = 1u << 2u,
        Unused_Bit3 = 1u << 3u,
        Unused_Bit4 = 1u << 4u,
        Unused_Bit5 = 1u << 5u,
        Unused_Bit6 = 1u << 6u,
        Unused_Bit7 = 1u << 7u,
    };

    AWE_ENUM(PacketDirection, uint8_t)
    {
        /// From server to client
        ToClient = 0,

        /// From client to server
        ToServer = 1
    };

    AWE_ENUM(ServerType, uint8_t)
    {
        Unknown = 0,

        /// Hosted on one GameServer instance
        StandaloneServer = 1,

        /// Multiple GameServer instances connected together
        Realm = 2
    };
}
