#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

namespace AWEngine::Packet
{
    typedef uint32_t ProtocolVersion_t;

    static const std::size_t GameName_Length = 8;
    typedef std::array<char, GameName_Length> GameName_t;

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

    AWE_ENUM(Direction, uint8_t)
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

    class ProtocolInfo
    {
    public:
        ProtocolInfo() = delete;

    public:
        static const uint16_t DefaultPort = 10101;
    public:
        static const ProtocolVersion_t ProtocolVersion;
        static const GameName_t GameName;

    public:
        template<std::size_t N>
        [[nodiscard]] static std::array<char, N> StringToArrayName(const std::string& strName);
    };
}
