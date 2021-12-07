#pragma once

#include <chrono>

// CMakeLists.txt will enable this automatically when https://github.com/nlohmann/json is found as a target `nlohmann_json` (must be created before this library).
#ifdef AWE_PACKET_LIB_JSON
#   include <nlohmann/json.hpp>
#endif

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/ProtocolInfo.hpp>
#include <AWEngine/Packet/Util/LocaleInfo.hpp>

namespace AWEngine::Packet::ToClient::Login
{
    /// Status response to `Init` packet.
    /// May be sent during gameplay (on server decision) to update those info.
    /// Format of the JSON is game-specific but in general should look like:
    /// {
    ///   "name": "Example Server",
    ///   "website": "http://example.com",
    ///   "players": {
    ///     "current": 6,
    ///     "max": 13
    ///   }
    /// }
    /// Try not to provide user-specific information (like usernames) as it would allow tracking of players.
    AWE_PACKET(ServerInfo, ToClient, 0x00)
    {
    public:
        explicit ServerInfo(
                std::array<char, 8> gameName,
                uint32_t            protocolVersion,
                Util::LocaleInfo    serverLocale,
                std::string         jsonString
        )
                :  GameName(gameName),
                   ProtocolVersion(protocolVersion),
                   ServerLocale(serverLocale),
                   JsonString(std::move(jsonString))
        {
        }

        explicit ServerInfo(const std::string& jsonString, Util::LocaleInfo serverLocale = {}) : ServerInfo(ProtocolInfo::GameName, ProtocolInfo::ProtocolVersion, serverLocale, jsonString) {}
#ifdef AWE_PACKET_LIB_JSON
        explicit ServerInfo(
                std::array<char, 8>   gameName,
                uint32_t              protocolVersion,
                const nlohmann::json& json
        ) : ServerInfo(gameName, protocolVersion, json.dump()) {}
        explicit ServerInfo(const nlohmann::json& json) : ServerInfo(ProtocolInfo::GameName, ProtocolInfo::ProtocolVersion, json) {}
#endif

        explicit ServerInfo(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
            in >> GameName >> ProtocolVersion >> ServerLocale >> JsonString;
        }

    public:
        std::array<char, 8> GameName;
        uint32_t            ProtocolVersion;
        Util::LocaleInfo    ServerLocale;
        std::string         JsonString;

#ifdef AWE_PACKET_LIB_JSON
    public:
        [[nodiscard]] inline nlohmann::json Json() const { return nlohmann::json::parse(JsonString); }
#endif

    public:
        void Write(PacketBuffer &out) const override
        {
            out << GameName << ProtocolVersion << ServerLocale << JsonString;
        }
    };
}
