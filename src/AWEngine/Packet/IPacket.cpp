#include "IPacket.hpp"

#include "PacketBuffer.hpp"
#include "PacketFlags.hpp"

#include "ToClient/Ping.hpp"
#include "ToClient/Login/ServerInfo.hpp"

#include "ToServer/Pong.hpp"
#include "ToServer/Login/Init.hpp"

namespace AWEngine::Packet
{
    bool IPacket::ReadPacket(std::istream& in, PacketHeader& header, PacketBuffer& buffer) noexcept
    {
        if(!in.good())
        {
            std::cerr << "Input stream is not in a good shape" << std::endl;
            return false;
        }

        in.read(reinterpret_cast<char*>(&header), sizeof(PacketHeader));
        header.Size = le16toh(header.Size);
        if(!in.good())
        {
            std::cerr << "Input stream is not in a good shape after reading header" << std::endl;
            return false;
        }

        if(header.Size > PacketBuffer::MaxSize)
        {
            std::cerr << "Packet size exceeded maximum allowed size" << std::endl;
            return false;
        }

        buffer.Clear();
        if(header.Size == 0)
            return true;

        try
        {
            if(header.Flags & PacketFlags::Compressed)
            {
                throw std::runtime_error("Compression is not supported at the time");
            }
            else
            {
                // Acts as `ClearAndLoad` as it is cleared above
                buffer.Load(in, header.Size);
                return true;
            }
        }
        catch(std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            return true;
        }
        catch(...)
        {
            return true;
        }
    }

    void IPacket::WritePacket(std::ostream& out, PacketID_t packetID, const PacketBuffer& buffer)
    {
        out.write(reinterpret_cast<const char*>(&packetID), sizeof(packetID));

        if(buffer.size() > 256 && false)
        {
            //TODO Compress the buffer, check if <= 90% of buffer.size() (if not, send uncompressed)
            throw std::runtime_error("Compression not implemented");

            // Compressed size
            uint16_t compressedSize = 0; //TODO Compressed data size
            out.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));

            // Flags
            PacketFlags flags = PacketFlags::Compressed;
            static_assert(sizeof(flags) == sizeof(uint8_t));
            out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

            // Uncompressed size
            uint16_t uncompressedSize = buffer.size();
            out.write(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));

            //TODO Write compressed data
        }
        else
        {
            // Data size
            uint16_t size = 0;
            out.write(reinterpret_cast<const char*>(&size), sizeof(size));

            // Flags
            PacketFlags flags = {};
            static_assert(sizeof(flags) == sizeof(uint8_t));
            out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

            out << buffer;
        }
    }
}
