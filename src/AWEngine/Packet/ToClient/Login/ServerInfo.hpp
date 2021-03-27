#pragma once

#include <chrono>

#include "../../IPacket.hpp"

namespace AWEngine::Packet::ToClient::Login
{
    AWE_PACKET(ServerInfo)
    {
    public:
        explicit ServerInfo(
                std::array<char, 8> gameName,
                uint32_t protocolVersion,
                std::string jsonString
        )
                :  GameName(gameName),
                   ProtocolVersion(protocolVersion),
                   JsonString(std::move(jsonString))
        {
        }

        explicit ServerInfo(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
            in >> GameName >> ProtocolVersion >> JsonString;
        }

    public:
        std::array<char, 8> GameName;
        uint32_t            ProtocolVersion;
        std::string         JsonString;

#ifdef AWE_PACKET_LIB_JSON
    public:
        inline nlohmann::json Json() const { nlohmann::json::parse(JsonString); }
#endif

    public:
        void Write(PacketBuffer &out) const override
        {
            out << GameName << ProtocolVersion << JsonString;
        }
    };
}
