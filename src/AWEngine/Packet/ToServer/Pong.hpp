#pragma once

#include "../IPacket.hpp"

namespace AWEngine::Packet::ToServer
{
    AWE_PACKET(Pong)
    {
    public:
        explicit Pong(uint64_t payload)
                : IPacket(Direction::ToServer, 0xFEu),
                  m_Payload(payload)
        {
        }

    public:
        uint64_t m_Payload;

    public:
        void Write(PacketBuffer &buffer) const override
        {
            buffer << static_cast<uint64_t>(m_Payload);
        }

    public:
        static std::shared_ptr<IPacket> Parse(PacketBuffer& buffer, PacketID_t id)
        {
            uint64_t payload;
            buffer >> payload;

            return std::make_shared<Pong>(payload);
        }
    };
}
