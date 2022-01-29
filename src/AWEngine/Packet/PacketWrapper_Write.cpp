#include "PacketWrapper.hpp"

#include <AWEngine/Packet/asio.hpp>

#include <stack_info.hpp>

namespace AWEngine::Packet
{
    std::vector<uint8_t> PacketWrapper::WritePacket(PacketID_t packetID, const PacketBuffer& buffer)
    {
        if(IPacket::CompressionThreshold != 0 && buffer.size() > IPacket::CompressionThreshold && false)
        {
            throw std::runtime_error("Compression not implemented");
            /*
            //TODO Compress the buffer, check if <= 90% of buffer.size() (if not, send uncompressed)
            // Packet ID
            out.write(reinterpret_cast<const char*>(&packetID), sizeof(packetID));

            // Compressed size
            uint16_t compressedSize = 0; //TODO Compressed data size
            out.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));

            // Flags
            PacketFlags flags = PacketFlags::Compressed;
            static_assert(sizeof(flags) == 1);
            out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

            // Uncompressed size
            uint16_t uncompressedSize = htole16(buffer.size());
            out.write(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));

            //TODO Write compressed data
             */
        }
        else
        {
            std::vector<uint8_t> outData(buffer.size() + 4 /* ID + length + flags */);

            // Packet ID
            static_assert(sizeof(PacketID_t) == 1);
            outData[0] = packetID;

            // Data size
            *reinterpret_cast<uint16_t*>(outData.data()+1) = htole16(buffer.size());

            // Flags
            PacketFlags flags = {};
            static_assert(sizeof(flags) == 1);
            outData[3] = static_cast<uint8_t>(flags);

            // Content
            buffer.Save(outData.data() + 4);

            return std::move(outData);
        }
    }
}
