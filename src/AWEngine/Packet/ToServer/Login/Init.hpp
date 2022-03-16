#pragma once

#include <chrono>

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/Util/LocaleInfo.hpp>
#include <AWEngine/Packet/ProtocolGameName.hpp>

namespace AWEngine::Packet::ToServer::Login
{
    enum class NextInitStep : uint8_t
    {
        ServerInfo = 0,
        Join       = 1
    };
    [[nodiscard]] inline std::string to_string(NextInitStep value)
    {
        switch(value)
        {
            case NextInitStep::ServerInfo:
                return "ServerInfo";
            case NextInitStep::Join:
                return "Join";
            default:
                throw std::runtime_error("Unexpected value");
        }
    }

    /// Initial packet send by client after the connection is created.
    /// If other packet is sent or no packet is sent in a short period of time (depends on server), server should terminate the connection.
    /// Tells server what game (and network version) the client is and whenever is just requesting info about the server or wants to join.
    /// Server may ignore Protocol Version and even Game Name when receiving request for info.
    /// If sent from outside of a game client, use correct Game Name (when looking for a specific game, otherwise 8 '\0' characters) and Protocol Version 0.
    /// Possible responses:
    /// - `ServerInfo` - the info client requested
    /// - `Kick` - no access to the info or no space for the client to join the server (maximum players online)
    /// - game-specific init packet - what server needs to tell you (auth info?)
    /// - (future idea) wait-in-line packet - keep-alive packet telling the client its position in queue
    template<typename TPacketID, TPacketID PacketID>
    class Init : public IPacket<TPacketID>
    {
    public:
        explicit Init(
            ProtocolGameName    gameName,
            ProtocolGameVersion gameVersion,
            Util::LocaleInfo    clientLocale,
            NextInitStep        next
        )
            : IPacket<TPacketID>(PacketID),
              GameName(gameName),
              GameVersion(gameVersion),
              ClientLocale(clientLocale),
              Next(next)
        {
        }

        explicit Init(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketID>(PacketID)
        {
            in >> GameName >> GameVersion >> ClientLocale >> reinterpret_cast<uint8_t&>(Next);
        }

    public:
        ProtocolGameName                   GameName;
        ProtocolGameVersion                GameVersion;
        AWEngine::Packet::Util::LocaleInfo ClientLocale;
        NextInitStep                       Next;

    public:
        inline void Write(PacketBuffer& out) const override
        {
            out << GameName << GameVersion << ClientLocale << static_cast<uint8_t>(Next);
        }
    };
}
