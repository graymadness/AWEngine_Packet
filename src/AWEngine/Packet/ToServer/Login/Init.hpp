#pragma once

#include <chrono>

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer::Login
{
    AWE_PACKET(Init, ToServer, 0x00)
    {
    public:
        AWE_ENUM(NextStep, uint8_t)
        {
            ServerInfo = 0,
            Join       = 1
        };
        inline static std::string to_string(NextStep value)
        {
            switch(value)
            {
                case NextStep::ServerInfo:
                    return "ServerInfo";
                case NextStep::Join:
                    return "Join";
                default:
                    throw std::runtime_error("Unexpected value");
            }
        }

    public:
        explicit Init(
                std::array<char, 8> gameName,
                uint32_t protocolVersion,
                NextStep next
        )
                :  GameName(gameName),
                   ProtocolVersion(protocolVersion),
                   Next(next)
        {
        }
        explicit Init(
                std::array<char, 8> gameName,
                NextStep next
        )
                :  GameName(gameName),
                   ProtocolVersion(ProtocolInfo::ProtocolVersion),
                   Next(next)
        {
        }

        explicit Init(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
            in >> GameName >> ProtocolVersion >> reinterpret_cast<uint8_t&>(Next);
        }

    public:
        std::array<char, 8> GameName;
        uint32_t            ProtocolVersion;
        NextStep            Next;

#ifdef AWE_PACKET_LIB_JSON
    public:
        inline nlohmann::json Json() const { nlohmann::json::parse(JsonString); }
#endif

    public:
        inline void Write(PacketBuffer& out) const override;
    };

    inline static PacketBuffer& operator<<(PacketBuffer& out, Init::NextStep value)
    {
        return out << static_cast<uint8_t>(value);
    }

    void Init::Write(PacketBuffer& out) const
    {
        out << GameName << ProtocolVersion << Next;
    }
}
