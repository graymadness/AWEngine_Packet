#pragma once

#include <AWEngine/Packet/IPacket.hpp>

namespace AWEngine::Packet::ToClient
{
    /// Packet sent to client before server terminates the connection.
    /// May contain no data if message is empty.
    template<typename TPacketEnum, TPacketEnum enumValue>
    class Kick : IPacket<TPacketEnum>
    {
    public:
        enum class MessageType : uint8_t
        {
            Raw = 0,
            Translatable = 1
        };

    public:
        explicit Kick(MessageType type, std::string message)
            : IPacket<TPacketEnum>(enumValue),
              Type(type),
              Message(std::move(message))
        {
        }
        explicit Kick(const std::string& message) : Kick(MessageType::Raw, message) {}
        explicit Kick() : Kick(MessageType::Raw, std::string()) {}

        explicit Kick(PacketBuffer& in) // NOLINT(cppcoreguidelines-pro-type-member-init)
            : IPacket<TPacketEnum>(enumValue, in)
        {
            if(in.empty())
            {
                Type = MessageType::Raw;
                Message = {};
            }
            else
            {
                in >> reinterpret_cast<uint8_t&>(Type) >> Message;
            }
        }

    public:
        MessageType Type;
        std::string Message;

    public:
        void Write(PacketBuffer& out) const override
        {
            if(!Message.empty())
                out << static_cast<uint8_t>(Type) << Message;
        }
    };
}
