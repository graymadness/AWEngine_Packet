#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer
{
    /// Packet sent to server before client terminates the connection to indicate that the disconnect was by decision of the user.
    template<typename TPacketEnum>
    AWE_PACKET(Disconnect, TPacketEnum)
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
