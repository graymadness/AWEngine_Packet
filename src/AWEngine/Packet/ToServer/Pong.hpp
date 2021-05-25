#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToServer
{
    AWE_PACKET(Pong, ToServer, 0xFE)
    {
    public:
        explicit Pong(uint64_t payload)
                : m_Payload(payload)
        {
        }

        explicit Pong(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
            in >> m_Payload;
        }

    public:
        uint64_t m_Payload;

    public:
        void Write(PacketBuffer &out) const override
        {
            out << static_cast<uint64_t>(m_Payload);
        }
    };
}
