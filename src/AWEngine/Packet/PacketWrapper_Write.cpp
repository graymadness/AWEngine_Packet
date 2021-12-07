#include "PacketWrapper.hpp"

#include <AWEngine/Packet/asio.hpp>

#include <stack_info.hpp>

// Uncomment to use `alloca` (allocate on stack) instead of big `char[]` variable.
// `alloca` will only allocate space on stack that it will need while `char[]` always uses the same.
//#define AWE_PACKET_ASIO_SEND_ALLOCA 1

namespace AWEngine::Packet
{
    void PacketWrapper::WritePacket(std::ostream& out, PacketID_t packetID, const PacketBuffer& buffer)
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
            // Packet ID
            static_assert(sizeof(PacketID_t) == 1);
            out.write(reinterpret_cast<const char*>(&packetID), sizeof(packetID));

            // Data size
            uint16_t size = htole16(buffer.size());
            out.write(reinterpret_cast<const char*>(&size), sizeof(size));

            // Flags
            PacketFlags flags = {};
            static_assert(sizeof(flags) == 1);
            out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));

            out << buffer;
        }
    }

    void PacketWrapper::WritePacket(asio::ip::tcp::socket& socket, PacketID_t packetID, const PacketBuffer& buffer)
    {
        if(IPacket::CompressionThreshold != 0 && buffer.size() > IPacket::CompressionThreshold && false)
        {
            throw std::runtime_error("Compression not implemented");
        }
        else
        {
            std::size_t totalSize = sizeof(packetID) + sizeof(uint16_t) + sizeof(PacketFlags) + buffer.size();
#if defined(AWE_PACKET_ASIO_SEND_ALLOCA) && AWE_PACKET_ASIO_SEND_ALLOCA == 1
            {
                std::size_t stackRemaining = stackavail();
                if(stackRemaining < totalSize)
                    throw std::runtime_error("Not enough space on stack");
                if(stackRemaining < PacketBuffer::MaxSize + sizeof(PacketHeader))
                    throw std::runtime_error("There would not be enough space on stack for largest possible packet");
            }
            char* buff = reinterpret_cast<char*>(alloca(totalSize)); // Allocate on stack as it is only temporary
#else
            char buff[PacketBuffer::MaxSize + sizeof(PacketHeader)];
#endif
            std::size_t buffOffset = 0;

            // Packet ID
            static_assert(sizeof(PacketID_t) == 1);
            *reinterpret_cast<PacketID_t*>(buff + buffOffset) = packetID;
            buffOffset += sizeof(packetID);

            // Data size
            uint16_t size = htole16(buffer.size());
            *reinterpret_cast<uint16_t*>(buff + buffOffset) = size;
            buffOffset += sizeof(size);

            // Flags
            PacketFlags flags = {};
            static_assert(sizeof(flags) == 1);
            *reinterpret_cast<PacketFlags*>(buff + buffOffset) = flags;
            buffOffset += sizeof(PacketFlags);

            //Packet content
            if(!buffer.empty())
            {
                std::copy(buffer.data(), buffer.data() + buffer.size(), buff + buffOffset);
                buffOffset += buffer.size();
            }

            assert(buffOffset == totalSize);
            asio::write(socket, asio::buffer(buff, totalSize));
        }
    }

    void PacketWrapper::WritePacketAsync(asio::ip::tcp::socket& socket, PacketID_t packetID, const PacketBuffer& buffer)
    {
        if(IPacket::CompressionThreshold != 0 && buffer.size() > IPacket::CompressionThreshold && false)
        {
            throw std::runtime_error("Compression not implemented");
        }
        else
        {
            std::size_t totalSize = sizeof(packetID) + sizeof(uint16_t) + sizeof(PacketFlags) + buffer.size();
#if defined(AWE_PACKET_ASIO_SEND_ALLOCA) && AWE_PACKET_ASIO_SEND_ALLOCA == 1
            {
                std::size_t stackRemaining = stackavail();
                if(stackRemaining < totalSize)
                    throw std::runtime_error("Not enough space on stack");
                if(stackRemaining < PacketBuffer::MaxSize + sizeof(PacketHeader))
                    throw std::runtime_error("There would not be enough space on stack for largest possible packet");
            }
            char* buff = reinterpret_cast<char*>(alloca(totalSize)); // Allocate on stack as it is only temporary
#else
            char buff[PacketBuffer::MaxSize + sizeof(PacketHeader)];
#endif
            std::size_t buffOffset = 0;

            // Packet ID
            static_assert(sizeof(PacketID_t) == 1);
            *reinterpret_cast<PacketID_t*>(buff + buffOffset) = packetID;
            buffOffset += sizeof(packetID);

            // Data size
            uint16_t size = htole16(buffer.size());
            *reinterpret_cast<uint16_t*>(buff + buffOffset) = size;
            buffOffset += sizeof(size);

            // Flags
            PacketFlags flags = {};
            static_assert(sizeof(flags) == 1);
            *reinterpret_cast<PacketFlags*>(buff + buffOffset) = flags;
            buffOffset += sizeof(PacketFlags);

            //Packet content
            if(!buffer.empty())
            {
                std::copy(buffer.data(), buffer.data() + buffer.size(), buff + buffOffset);
                buffOffset += buffer.size();
            }

            assert(buffOffset == totalSize);
            asio::async_write(
                    socket,
                    asio::buffer(buff, totalSize),
                    [totalSize](const asio::error_code& ec, std::size_t bytesTransferred)
                    {
                        if(!ec)
                            std::cerr << "Failed to write " << (totalSize - bytesTransferred) << " bytes" << std::endl;
                    }
                             );
        }
    }

#ifdef AWE_PACKET_COROUTINE
    asio::awaitable<void> PacketWrapper::WritePacketAsync(asio::tcp_socket& socket, PacketID_t packetID, const PacketBuffer& buffer)
    {
        if(IPacket::CompressionThreshold != 0 && buffer.size() > IPacket::CompressionThreshold && false)
        {
            throw std::runtime_error("Compression not implemented");
        }
        else
        {
            std::size_t totalSize = sizeof(packetID) + sizeof(uint16_t) + sizeof(PacketFlags) + buffer.size();
#if defined(AWE_PACKET_ASIO_SEND_ALLOCA) && AWE_PACKET_ASIO_SEND_ALLOCA == 1
            {
                std::size_t stackRemaining = stackavail();
                if(stackRemaining < totalSize)
                    throw std::runtime_error("Not enough space on stack");
                if(stackRemaining < PacketBuffer::MaxSize + sizeof(PacketHeader))
                    throw std::runtime_error("There would not be enough space on stack for largest possible packet");
            }
            char* buff = reinterpret_cast<char*>(alloca(totalSize)); // Allocate on stack as it is only temporary
#else
            char buff[PacketBuffer::MaxSize + sizeof(PacketHeader)];
#endif
            std::size_t buffOffset = 0;

            // Packet ID
            static_assert(sizeof(PacketID_t) == 1);
            *reinterpret_cast<PacketID_t*>(buff + buffOffset) = packetID;
            buffOffset += sizeof(packetID);

            // Data size
            uint16_t size = htole16(buffer.size());
            *reinterpret_cast<uint16_t*>(buff + buffOffset) = size;
            buffOffset += sizeof(size);

            // Flags
            PacketFlags flags = {};
            static_assert(sizeof(flags) == 1);
            *reinterpret_cast<PacketFlags*>(buff + buffOffset) = flags;
            buffOffset += sizeof(PacketFlags);

            //Packet content
            if(!buffer.empty())
            {
                std::copy(buffer.data(), buffer.data() + buffer.size(), buff + buffOffset);
                buffOffset += buffer.size();
            }

            assert(buffOffset == totalSize);
            co_await asio::async_write(socket, asio::buffer(buff, totalSize));
        }
    }
#endif
}
