#include "IPacket.hpp"

#include "PacketBuffer.hpp"

#include "ToClient/Ping.hpp"

#include "ToServer/Pong.hpp"

namespace AWEngine::Packet
{
    std::map<PacketID_t, PacketParser_t> IPacket::s_Packets_ToClient =  // NOLINT(cert-err58-cpp)
    {
            {
                    0xFEu,
                    ToClient::Ping::Parse
            },
            {
                    0xFFu,
                    [](PacketBuffer& buffer, PacketID_t id) -> std::shared_ptr<IPacket> { throw std::runtime_error("Attempted to parse dummy 0xFF Packet ID"); }
            }
    };
    std::map<PacketID_t, PacketParser_t> IPacket::s_Packets_ToServer = // NOLINT(cert-err58-cpp)
    {
            {
                    0xFEu,
                    ToServer::Pong::Parse
            },
            {
                    0xFFu,
                    [](PacketBuffer& buffer, PacketID_t id) -> std::shared_ptr<IPacket> { throw std::runtime_error("Attempted to parse dummy 0xFF Packet ID"); }
            }
    };

    std::shared_ptr<IPacket> IPacket::Read(std::istream& in, Direction direction)
    {
        auto& packets = GetParserFromDirection(direction);

        PacketID_t id;
        in.read(reinterpret_cast<char*>(&id), sizeof(id));

        uint32_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));

        if(size > PacketBuffer::MaxSize)
            throw std::runtime_error("Packet size exceeded maximum allowed size");

        auto& parser = packets[id];
        auto buffer = PacketBuffer(in, size);

        return parser(buffer, id);
    }
}
