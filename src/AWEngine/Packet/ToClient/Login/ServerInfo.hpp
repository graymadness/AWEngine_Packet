#pragma once

#include <chrono>
#include <optional>

// CMakeLists.txt will enable this automatically when https://github.com/nlohmann/json is found as a target `nlohmann_json` (must be created before this library).
#ifdef AWE_PACKET_LIB_JSON
#   include <nlohmann/json.hpp>
#endif

#include "AWEngine/Packet/IPacket.hpp"
#include "AWEngine/Packet/ProtocolInfo.hpp"
#include "AWEngine/Packet/Util/LocaleInfo.hpp"
#include "AWEngine/Packet/ProtocolGameName.hpp"

namespace AWEngine::Packet::ToClient::Login
{
    struct ServerInfo_Utils
    {
    public:
        ServerInfo_Utils() = delete;

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
    };

    template<typename T>
    T SimpleJsonParse(const std::string& json, const std::string& key) noexcept = delete;

    /// Looks for this pattern inside JSON: "key": "value"
    /// Supports
    /// Returns empty string on invalid JSON or when not found
    template<>
    std::string SimpleJsonParse<std::string>(const std::string& json, const std::string& key) noexcept;
    /// Returns 0 on error or empty value
    template<>
    int SimpleJsonParse<int>(const std::string& json, const std::string& key) noexcept;
    /// Returns bool based on first character of value (will return `true` for "yogurt").
    /// No value is returned on error or unknown character.
    template<>
    std::optional<bool> SimpleJsonParse<std::optional<bool>>(const std::string& json, const std::string& key) noexcept;
    /// Returns 0 on error or empty value
    template<>
    std::optional<int> SimpleJsonParse<std::optional<int>>(const std::string& json, const std::string& key) noexcept;

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
    template<typename TPacketID, TPacketID PacketID>
    class ServerInfo : public IPacket<TPacketID>
    {
    public:
        explicit ServerInfo(
            ProtocolGameName    gameName,
            ProtocolGameVersion gameVersion,
            std::string         jsonString
        )
            : IPacket<TPacketID>(PacketID),
              GameName(gameName),
              GameVersion(gameVersion),
              JsonString(std::move(jsonString))
        {
        }

#ifdef AWE_PACKET_LIB_JSON
        explicit ServerInfo(
            ProtocolGameName      gameName,
            ProtocolGameVersion   gameVersion,
            const nlohmann::json& json
        ) : ServerInfo(gameName, protocolVersion, json.dump()) {}
        explicit ServerInfo(const nlohmann::json& json) : ServerInfo(ProtocolInfo::GameName, ProtocolInfo::ProtocolVersion, json) {}
#endif

        explicit ServerInfo(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketID>(PacketID)
        {
            in >> GameName >> GameVersion >> JsonString;
        }

    public:
        ProtocolGameName    GameName;
        ProtocolGameVersion GameVersion;
        std::string         JsonString;

#ifdef AWE_PACKET_LIB_JSON
    public:
        [[nodiscard]] inline nlohmann::json Json() const { return nlohmann::json::parse(JsonString); }
#endif

    public:
        [[nodiscard]] inline std::string         MOTD()          const noexcept { return SimpleJsonParse<std::string>(        JsonString, ServerInfo_Utils::Field_MOTD         ); }
        [[nodiscard]] inline std::string         ServerName()    const noexcept { return SimpleJsonParse<std::string>(        JsonString, ServerInfo_Utils::Field_ServerName   ); }
        [[nodiscard]] inline std::string         Website()       const noexcept { return SimpleJsonParse<std::string>(        JsonString, ServerInfo_Utils::Field_Website      ); }
        [[nodiscard]] inline Util::LocaleInfo    ServerLocale()  const noexcept { return SimpleJsonParse<std::string>(        JsonString, ServerInfo_Utils::Field_ServerLocale ); }
        [[nodiscard]] inline std::optional<int>  OnlinePlayers() const noexcept { return SimpleJsonParse<int>(                JsonString, ServerInfo_Utils::Field_OnlinePlayers); }
        [[nodiscard]] inline int                 MaxPlayers()    const noexcept { return SimpleJsonParse<int>(                JsonString, ServerInfo_Utils::Field_MaxPlayers   ); }
        [[nodiscard]] inline std::optional<bool> Whitelist()     const noexcept { return SimpleJsonParse<std::optional<bool>>(JsonString, ServerInfo_Utils::Field_Whitelist    ); }

    public:
        void Write(PacketBuffer &out) const override
        {
            out << GameName << GameVersion << JsonString;
        }
    };
}
