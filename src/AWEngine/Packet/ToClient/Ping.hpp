#pragma once

#include <chrono>

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToClient
{
    AWE_PACKET(Ping, ToClient, 0xFE)
    {
    public:
        explicit Ping(uint64_t payload)
                : m_Payload(payload)
        {
        }
        /// New instance with current time
        explicit Ping()
                : Ping(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
        {
        }

        explicit Ping(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        {
            in >> m_Payload;
        }

    public:
        uint64_t m_Payload;

    public:
        void Write(PacketBuffer& out) const override
        {
            out << static_cast<uint64_t>(m_Payload);
        }
    };
}
