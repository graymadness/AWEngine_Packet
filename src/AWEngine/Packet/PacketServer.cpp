#include <iomanip>
#include "PacketServer.hpp"

#include "PacketBuffer.hpp"
#include "IPacket.hpp"

namespace AWEngine::Packet
{
    template<typename TPlayer>
    void PacketServer<TPlayer>::Player_t::SendPacket(const std::shared_ptr<IPacket>& packet)
    {
        if(m_Server == nullptr)
            throw std::runtime_error("Failed to send packet - NULL server");

        PacketBuffer buffer;
        buffer << packet->m_ID;
        buffer << static_cast<uint16_t>(0); // placeholder for size
        buffer << static_cast<uint8_t>(0); // PacketFlags

        packet->Write(buffer);

        uint16_t& dataSize = reinterpret_cast<uint16_t&>(buffer[1]);

        asio::post(
            m_Server->Tcp().Context(),
            [this, buffer]()
            {
                // If there are items in the queue, it is already being processed by ProcessQueue().
                // Otherwise start the processing
                bool outQueueEmpty = m_OutQueue.empty();
                m_OutQueue.push_back(buffer.Buffer());
                if (outQueueEmpty)
                {
                    ProcessQueue();
                }
            }
        );
    }

    template<typename TPlayer>
    void PacketServer<TPlayer>::Player_t::ProcessQueue()
    {
        auto& buff = m_OutQueue.peek_front();
        asio::async_write(
            m_Socket,
            asio::buffer(buff.data(), buff.size()),
            [this](std::error_code ec, std::size_t length)
            {
                // Message has been sent

                //TODO Lock once

                auto buff = m_OutQueue.pop_front();

                if (ec)
                {
                    auto server = m_Server.lock();

                    std::cout << server->m_TcpServer.ServerPrefix() << ": Failed to write packet ";
                    std::cout << "0x"
                              << std::setfill('0') << std::setw(sizeof(uint8_t)*2)
                              << std::hex << static_cast<uint32_t>(buff[0]);
                    std::cout << " to " << m_Socket->remote_endpoint() << std::endl;
                    m_Socket->close();

                    server->DisconnectCallback(this->shared_from_this());
                }
                else
                {
                    // More data to process
                    if (!m_OutQueue.empty())
                        ProcessQueue();
                }
            }
        );
    }
}
