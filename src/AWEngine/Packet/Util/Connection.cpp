#include "Connection.hpp"

namespace AWEngine::Packet::Util
{
    void Connection::WriteHeader()
    {
        // If this function is called, we know the outgoing message queue must have at least one message to send.
        // So allocate a transmission buffer to hold the message, and issue the work - asio, send these bytes
        asio::async_write(
            m_socket,
            asio::buffer(&m_qMessagesOut.peek_front().Header, sizeof(PacketSendInfo::Header)),
            [this](std::error_code ec, std::size_t length)
            {
                // asio has now sent the bytes - if there was a problem an error would be available...
                if(!ec)
                {
                    // ... no error, so check if the message header just sent also has a message body...
                    if(!m_qMessagesOut.peek_front().Body.empty())
                    {
                        // ...it does, so issue the task to write the body bytes
                        WriteBody();
                    }
                    else
                    {
                        // ...it didnt, so we are done with this message.
                        // Remove it from the outgoing message queue
                        m_qMessagesOut.pop_front();

                        // If the queue is not empty, there are more messages to send, so make this happen by issuing the task to send the next header.
                        if(!m_qMessagesOut.empty())
                        {
                            WriteHeader();
                        }
                    }
                }
                else
                {
                    // ...asio failed to write the message, we could analyse why but for now simply assume the connection has died by closing the socket.
                    // When a future attempt to write to this client fails due to the closed socket, it will be tidied up.
                    std::cerr << "Write Header Fail." << std::endl;
                    m_socket.close();
                }
            }
        );
    }

    void Connection::WriteBody()
    {
        // If this function is called, a header has just been sent, and that header indicated a body existed for this message.
        // Fill a transmission buffer with the body data, and send it!
        asio::async_write(
            m_socket,
            asio::buffer(m_qMessagesOut.peek_front().Body.data(), m_qMessagesOut.peek_front().Body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if(!ec)
                {
                    // Sending was successful, so we are done with the message and remove it from the queue
                    m_qMessagesOut.pop_front();

                    // If the queue still has messages in it, then issue the task to send the next messages' header.
                    if(!m_qMessagesOut.empty())
                    {
                        WriteHeader();
                    }
                }
                else
                {
                    // Sending failed, see WriteHeader() equivalent for description :P
                    std::cerr << "Write Body Fail." << std::endl;
                    m_socket.close();
                }
            }
        );
    }

    void Connection::ReadHeader()
    {
        // If this function is called, we are expecting asio to wait until it receives enough bytes to form a header of a message.
        // We know the headers are a fixed size, so allocate a transmission buffer large enough to store it.
        // In fact, we will construct the message in a "temporary" message object as it's convenient to work with.
        asio::async_read(
            m_socket,
            asio::buffer(&m_WipInMessage.Header, sizeof(PacketSendInfo::Header)),
            [this](std::error_code ec, std::size_t length)
            {
                if(!ec)
                {
                    if(m_WipInMessage.Header.Size == 0)
                    {
                        // Message without body, add it to received queue
                        AddToIncomingMessageQueue();
                    }
                    else
                    {
                        // Read its body
                        ReadBody();
                    }
                }
                else
                {
                    // Reading from the client went wrong, most likely a disconnect has occurred.
                    // Close the socket and let the system tidy it up later.
                    std::cerr << "Read Header Fail." << std::endl;
                    m_socket.close();
                }
            }
        );
    }

    void Connection::ReadBody()
    {
        m_WipInMessage.Body.resize(m_WipInMessage.Header.Size);

        // If this function is called, a header has already been read, and that header request we read a body.
        // The space for that body has already been allocated in the temporary message object, so just wait for the bytes to arrive...
        asio::async_read(
            m_socket,
            asio::buffer(m_WipInMessage.Body.data(), m_WipInMessage.Body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if(!ec)
                {
                    if(m_WipInMessage.Header.Flags & PacketFlags::Compressed)
                    {
                        std::cerr << "Compression not implemented." << std::endl;
                        m_socket.close();
                    }
                    else
                    {
                        // Message is complete, process it
                        AddToIncomingMessageQueue();
                    }
                }
                else
                {
                    std::cerr << "Read Body Fail." << std::endl;
                    m_socket.close();
                }
            }
        );
    }

    void Connection::AddToIncomingMessageQueue()
    {
        switch(m_WipInMessage.Header.ID)
        {
            default:
            {
                m_qMessagesIn.push_back(m_WipInMessage);

                // We must now prime the asio context to receive the next message.
                // It wil just sit and wait for bytes to arrive, and the message construction process repeats itself.
                // Clever huh?
                ReadHeader();
                break;
            }


        }
    }

    void Connection::ConnectToServer(const asio::ip::basic_resolver<asio::ip::tcp, asio::any_io_executor>::results_type& endpoints)
    {
        if(m_Direction == PacketDirection::ToClient)
            throw std::runtime_error("Incorrect direction");

        // Request asio attempts to connect to an endpoint
        asio::async_connect(
            m_socket,
            endpoints,
            [this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint)
            {
                if (!ec)
                {
                    ReadHeader();
                }
                else
                {
                    m_Status = ConnectionStatus::Disconnected;

                    std::cerr << "Failed to connect to the server (" << ec.value() << "): " << ec.message() << std::endl;
                }
            }
        );
    }

    void Connection::Disconnect()
    {
        if (IsConnected())
            asio::post(m_asioContext, [this]() { m_socket.close(); });
    }
}
