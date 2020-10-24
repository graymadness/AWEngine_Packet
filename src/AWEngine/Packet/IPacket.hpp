#pragma once

#include <memory>
#include <map>
#include <functional>

#include "Direction.hpp"
#include "PacketBuffer.hpp"

namespace AWEngine::Packet
{
    class IPacket;

    typedef uint8_t PacketID_t;
    typedef std::function<std::shared_ptr<IPacket>(PacketBuffer&, PacketID_t)> PacketParser_t;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
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
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
        const Direction m_Destination;
        const PacketID_t m_ID;
#pragma clang diagnostic pop

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
#pragma clang diagnostic pop
}
