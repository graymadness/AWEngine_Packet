#pragma once

#include <chrono>

#include "../IPacket.hpp"

namespace AWEngine::Packet::ToClient
{
    AWE_PACKET(Ping)
    {
    public:
        explicit Ping(uint64_t payload)
                : IPacket(Direction::ToClient, 0xFEu),
                  m_Payload(payload)
        {
        }
        /// New instance with current time
        explicit Ping()
                : Ping(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
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

            return std::make_shared<Ping>(payload);
        }
    };
}
