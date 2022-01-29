#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer
{
    /// Packet sent to server before client terminates the connection to indicate that the disconnect was by decision of the user.
    template<typename TPacketEnum, TPacketEnum enumValue>
    class Disconnect : IPacket<TPacketEnum>
    {
    public:
        explicit Disconnect()
            : IPacket<TPacketEnum>(enumValue)
        {

        }

        explicit Disconnect(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketEnum>(enumValue, in)
        {
        }

    public:
        void Write(PacketBuffer& out) const override
        {
        }
    };
}
