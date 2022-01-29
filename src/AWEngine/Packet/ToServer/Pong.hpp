#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer
{
    /// Response to `Ping` packet from the server.
    /// Do not try to guess the `Payload`.
    /// Sending multiple `Pong` packets after receiving `Ping` packet may result in termination of the connection by the server.
    template<typename TPacketEnum>
    AWE_PACKET(Pong, TPacketEnum)
    {
    public:
        explicit Pong(uint64_t payload)
                : Payload(payload)
        {
        }

        explicit Pong(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
            in >> Payload;
        }

    public:
        uint64_t Payload;

    public:
        void Write(PacketBuffer& out) const override
        {
            out << static_cast<uint64_t>(Payload);
        }
    };
}
