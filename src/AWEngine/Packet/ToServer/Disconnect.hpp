#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer
{
    /// Packet sent to server before client terminates the connection to indicate that the disconnect was by decision of the user.
    AWE_PACKET(Disconnect, ToServer, 0xFF)
    {
    public:
        explicit Disconnect() = default;

        explicit Disconnect(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
        }

    public:
        void Write(PacketBuffer& out) const override
        {
        }
    };
}
