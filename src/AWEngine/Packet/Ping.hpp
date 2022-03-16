#pragma once

#include <chrono>

#include "IPacket.hpp"

namespace AWEngine::Packet
{
    /// Server requesting immediate response from the client to measure the delay.
    ///
    /// Server expects Pong response with same `Payload` otherwise should terminates the connection.
    /// Client can (and should) use this same class to respond back.
    ///
    /// The payload is usually current UNIX time but it is not required.
    /// `PacketClient` responds to this packet automatically.
    template<typename TPacketID, TPacketID PacketID>
    class Ping : public IPacket<TPacketID>
    {
    public:
        explicit Ping(uint64_t payload)
            : IPacket<TPacketID>(PacketID),
              Payload(payload)
        {
        }
        /// New instance with current time
        explicit Ping()
            : Ping(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count())
        {
        }

        explicit Ping(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketID>(PacketID)
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
