#pragma once

#include <chrono>

// CMakeLists.txt will enable this automatically when https://github.com/nlohmann/json is found as a target `nlohmann_json` (must be created before this library).
#ifdef AWE_PACKET_LIB_JSON
#   include <nlohmann/json.hpp>
#endif

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/ProtocolInfo.hpp>

namespace AWEngine::Packet::ToClient::Login
{
    /// Status response to `Init` packet.
    /// May be sent during gameplay (on server decision) to update those info.
    /// Format of the JSON is game-specific but in general should look like:
    /// {
    ///   "name": "Example Server",
    ///   "url": "http://example.com",
    ///   "players": {
    ///     "current": 6,
    ///     "max": 13
    ///   }
    /// }
    AWE_PACKET(ServerInfo, ToClient, 0x00)
    {
    public:
        explicit ServerInfo(
                std::array<char, 8> gameName,
                uint32_t            protocolVersion,
                std::string         jsonString
        )
                :  GameName(gameName),
                   ProtocolVersion(protocolVersion),
                   JsonString(std::move(jsonString))
        {
        }

        explicit ServerInfo(const std::string& jsonString) : ServerInfo(ProtocolInfo::GameName, ProtocolInfo::ProtocolVersion, jsonString) {}
#ifdef AWE_PACKET_LIB_JSON
        explicit ServerInfo(
                std::array<char, 8> gameName,
                uint32_t            protocolVersion,
                nlohmann::json      json
        ) : ServerInfo(gameName, protocolVersion, json.dump()) {}
        explicit ServerInfo(nlohmann::json json) : ServerInfo(ProtocolInfo::GameName, ProtocolInfo::ProtocolVersion, json) {}
#endif

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
