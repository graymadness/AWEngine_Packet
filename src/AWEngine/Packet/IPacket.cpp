#include "IPacket.hpp"

#include "PacketBuffer.hpp"
#include "PacketFlags.hpp"

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

    struct PacketHeader
    {
        PacketID_t ID;
        uint16_t Size;
        PacketFlags Flags;
    };

    std::shared_ptr<IPacket> IPacket::Read(std::istream& in, Direction direction)
    {
        auto& packets = GetParserFromDirection(direction);

        PacketHeader header = {};
        in.read(reinterpret_cast<char*>(&header), sizeof(PacketHeader));
        //header.Size = le16toh(header.Size);

        if(header.Size > PacketBuffer::MaxSize)
            throw std::runtime_error("Packet size exceeded maximum allowed size");

        PacketBuffer buffer;
        if(header.Flags & PacketFlags::Compressed)
        {
            throw std::runtime_error("Compression is not supported at the time");
        }
        else
        {
            buffer = PacketBuffer(in, header.Size);
        }

        auto& parser = packets[header.ID];
        //TODO Check for non-existent parser + runtime_error
        return parser(buffer, header.ID);
    }
}
