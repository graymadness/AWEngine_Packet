#pragma once

#include <memory>
#include <map>
#include <functional>

#include "Direction.hpp"

namespace AWEngine::Packet
{
    class PacketBuffer;
    class IPacket;

    typedef uint8_t PacketID_t;
    typedef std::function<std::shared_ptr<IPacket>(PacketBuffer&, PacketID_t)> PacketParser_t;

    class IPacket
    {
    public:
        IPacket(Direction destination, PacketID_t id)
         : m_Destination(destination), m_ID(id)
        {
        }

    public:
        virtual ~IPacket() = default;

    public:
        const Direction m_Destination;
        const PacketID_t m_ID;

    public:
        virtual void Write(PacketBuffer& buffer) const = 0;

    public:
        static std::shared_ptr<IPacket> Read(std::istream& in, Direction direction);
        static void RegisterParser(Direction direction, PacketID_t id, const PacketParser_t& parser) { GetParserFromDirection(direction).emplace(id, parser); }
    private:
        static std::map<PacketID_t, PacketParser_t> s_Packets_ToClient;
        static std::map<PacketID_t, PacketParser_t> s_Packets_ToServer;
        static std::map<PacketID_t, PacketParser_t>& GetParserFromDirection(Direction direction) { return direction == Direction::ToClient ? s_Packets_ToClient : s_Packets_ToServer; }

    };
}
