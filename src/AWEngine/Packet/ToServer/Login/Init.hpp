#pragma once

#include <chrono>

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/Util/LocaleInfo.hpp>

namespace AWEngine::Packet::ToServer::Login
{
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
        enum class NextStep : uint8_t
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
            uint32_t            protocolVersion,
            Util::LocaleInfo    clientLocale,
            NextStep            next
        )
            : IPacket<TPacketID>(PacketID),
            GameName(gameName),
               ProtocolVersion(protocolVersion),
               ClientLocale(clientLocale),
               Next(next)
        {
        }

        explicit Init(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketID>(PacketID, in)
        {
            in >> GameName >> ProtocolVersion >> ClientLocale >> reinterpret_cast<uint8_t&>(Next);
        }

    public:
        std::array<char, 8>                GameName;
        uint32_t                           ProtocolVersion;
        AWEngine::Packet::Util::LocaleInfo ClientLocale;
        NextStep                           Next;

    public:
        inline void Write(PacketBuffer& out) const override
        {
            out << GameName << ProtocolVersion << ClientLocale << static_cast<uint8_t>(Next);
        }
    };
}
