#pragma once

#include <chrono>
#include <optional>

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
    /// Format of the JSON is game-specific but those are the generic ones:
    /// {
    ///   "name": "Example Server",
    ///   "website": "http://example.com",
    ///   "motd": "Open to everyone!\nCome and join",
    ///   "players": {
    ///     "online": 6,
    ///     "max": 13
    ///   },
    ///   "locale": "en"
    /// }
    /// Try not to provide user-specific information (like usernames) as it would allow tracking of players.
    /// It is recommended to generate the JSON once (and store it as string) and re-use it for every packet.
    AWE_PACKET(ServerInfo, ToClient, 0x00)
    {
    public:
        /// Message of the day.
        /// Type: text
        static const std::string Field_MOTD;
        /// Name of the server.
        /// Type: text
        static const std::string Field_ServerName;
        /// Website associated with the server.
        /// Type: text
        static const std::string Field_Website;
        /// Localization info about the server.
        /// Represents language (and dialect/country) but not geological location.
        /// Type: text
        static const std::string Field_ServerLocale;
        /// Current number of online players.
        /// Type: integer
        static const std::string Field_OnlinePlayers;
        /// Maximum number of players at the same time.
        /// Type: integer
        static const std::string Field_MaxPlayers;
        /// Does server limit players that can join?
        /// Type: boolean (text: yes / no, default: no)
        static const std::string Field_Whitelist;

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
                std::array<char, 8>   gameName,
                uint32_t              protocolVersion,
                const nlohmann::json& json
        ) : ServerInfo(gameName, protocolVersion, json.dump()) {}
        explicit ServerInfo(const nlohmann::json& json) : ServerInfo(ProtocolInfo::GameName, ProtocolInfo::ProtocolVersion, json) {}
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
        [[nodiscard]] inline nlohmann::json Json() const { return nlohmann::json::parse(JsonString); }
#endif

    public:
        [[nodiscard]] std::string         MOTD()          const noexcept;
        [[nodiscard]] std::string         ServerName()    const noexcept;
        [[nodiscard]] std::string         Website()       const noexcept;
        [[nodiscard]] Util::LocaleInfo    ServerLocale()  const noexcept;
        [[nodiscard]] std::optional<int>  OnlinePlayers() const noexcept;
        [[nodiscard]] int                 MaxPlayers()    const noexcept;
        [[nodiscard]] std::optional<bool> Whitelist()     const noexcept;

    public:
        void Write(PacketBuffer &out) const override
        {
            out << GameName << ProtocolVersion << JsonString;
        }
    };
}
