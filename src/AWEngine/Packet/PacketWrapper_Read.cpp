#include "PacketWrapper.hpp"

namespace AWEngine::Packet
{
    bool PacketWrapper::ReadPacket(std::istream& in, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept
    {
        everythingOk = false;

        if(!in.good())
        {
            std::cerr << "Input stream is not in a good shape" << std::endl;
            return false;
        }

        PacketHeader header;
        in.read(reinterpret_cast<char*>(&header), sizeof(PacketHeader));

        packetID = header.ID;

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
                everythingOk = true;
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

    bool PacketWrapper::ReadPacket(asio::ip::tcp::socket& socket, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept
    {
        everythingOk = false;

        PacketHeader header;
        std::size_t bytesTransferred = asio::read(socket, asio::buffer(reinterpret_cast<char*>(&header), sizeof(PacketHeader)));
        if(bytesTransferred != sizeof(PacketHeader))
        {
            std::cerr << "Did not read enough bytes" << std::endl;
            return false;
        }

        packetID = header.ID;

        header.Size = le16toh(header.Size);
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
                buffer.Load(socket, header.Size);
                everythingOk = true;
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

    void PacketWrapper::ReadPacketAsync(
            asio::ip::tcp::socket& socket,
            std::function<void(PacketID_t, PacketBuffer)> receivedCallback,
            std::function<void(std::string)> failedCallback
            ) noexcept
    {
        PacketHeader header; // NOLINT(cppcoreguidelines-pro-type-member-init)
        asio::async_read(
                socket,
                asio::buffer(&header, sizeof(PacketHeader)),
                [&socket, &header, &receivedCallback, &failedCallback](const asio::error_code& ec, std::size_t bytesTransferred)
                {
                    if(bytesTransferred != sizeof(PacketHeader))
                    {
                        if(failedCallback)
                            failedCallback("Did not read enough bytes");
                        return;
                    }

                    PacketID_t packetID = header.ID;

                    header.Size = le16toh(header.Size);
                    if(header.Size > PacketBuffer::MaxSize)
                    {
                        if(failedCallback)
                            failedCallback("Packet size exceeded maximum allowed size");
                        return;
                    }

                    if(header.Size == 0)
                    {
                        if(receivedCallback)
                            receivedCallback(packetID, {});
                        return;
                    }

                    if(header.Flags & PacketFlags::Compressed)
                    {
                        if(failedCallback)
                            failedCallback("Compression is not supported at the time");
                        return;
                    }
                    else
                    {
                        if(!receivedCallback)
                            return;

                        PacketBuffer buffer = {};
                        buffer.reserve(header.Size);
                        assert(buffer.size() == header.Size);

                        asio::async_read(
                                socket,
                                asio::buffer(reinterpret_cast<char*>(buffer.data()), header.Size),
                                [packetID, &buffer, &receivedCallback, &failedCallback](const asio::error_code& ec, std::size_t bytesTransferred)
                                {
                                    if(bytesTransferred != buffer.size())
                                    {
                                        if(failedCallback)
                                            failedCallback("Did not read enough bytes");
                                        return;
                                    }

                                    if(receivedCallback)
                                        receivedCallback(packetID, buffer);

                                }
                                        );
                    }
                }
                );
    }

#ifdef AWE_PACKET_COROUTINE
    asio::awaitable<bool> PacketWrapper::ReadPacketAsync(asio::tcp_socket& socket, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept
    {
        everythingOk = false;

        PacketHeader header;

        try
        {
            std::size_t bytesRead = 0;
            while(bytesRead < sizeof(PacketHeader))
            {
                co_await socket.async_wait(asio::socket_base::wait_read);
                bytesRead += co_await socket.async_read_some(asio::buffer(reinterpret_cast<char*>(&header) + bytesRead, sizeof(PacketHeader) - bytesRead));
            }
        }
        catch(std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            co_return true;
        }
        catch(...)
        {
            co_return true;
        }

        packetID = header.ID;

        header.Size = le16toh(header.Size);
        if(header.Size > PacketBuffer::MaxSize)
        {
            std::cerr << "Packet size exceeded maximum allowed size" << std::endl;
            co_return false;
        }

        buffer.Clear();
        if(header.Size == 0)
            co_return true;

        try
        {
            if(header.Flags & PacketFlags::Compressed)
            {
                throw std::runtime_error("Compression is not supported at the time");
            }
            else
            {
                // Acts as `ClearAndLoad` as it is cleared above
                co_await buffer.LoadAsync(socket, header.Size);
                everythingOk = true;
                co_return true;
            }
        }
        catch(std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            co_return true;
        }
        catch(...)
        {
            co_return true;
        }
    }
#endif
}
