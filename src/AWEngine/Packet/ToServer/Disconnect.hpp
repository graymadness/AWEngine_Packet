#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer
{
    /// Packet sent to server before client terminates the connection to indicate that the disconnect was by decision of the user.
    template<typename TPacketID, TPacketID PacketID>
    class Disconnect : public IPacket<TPacketID>
    {
    public:
        explicit Disconnect()
            : IPacket<TPacketID>(PacketID)
        {

        }

        explicit Disconnect(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketID>(PacketID)
        {
        }

    public:
        void Write(PacketBuffer& out) const override
        {
        }
    };
}
