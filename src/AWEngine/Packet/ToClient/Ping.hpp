#pragma once

#include <chrono>

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToClient
{
    /// Server requesting immediate response from the client to measure the delay.
    /// Server expects Pong response with same `Payload` otherwise should terminates the connection.
    /// The payload is usually current UNIX time but it is not required.
    /// `PacketClient` responds to this packet automatically.
    template<typename TPacketEnum, TPacketEnum enumValue>
    class Ping : IPacket<TPacketEnum>
    {
    public:
        explicit Ping(uint64_t payload)
            : IPacket<TPacketEnum>(enumValue),
              Payload(payload)
        {
        }
        /// New instance with current time
        explicit Ping()
            : Ping(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
        {
        }

        explicit Ping(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketEnum>(enumValue, in)
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
