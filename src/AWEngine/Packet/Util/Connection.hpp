#pragma once
#include "AWEngine/Packet/Util/Core_Packet.hpp"

#include <asio.hpp>

#include "AWEngine/Packet/IPacket.hpp"
#include "ThreadSafeQueue.hpp"
#include "AWEngine/Packet/PacketBuffer.hpp"

namespace AWEngine::Packet::Util
{
    template<typename TPacketEnum>
    class Connection : public std::enable_shared_from_this<Connection<TPacketEnum>>
    {
    public:
        struct PacketSendInfo
        {
            PacketHeader<TPacketEnum> Header;
            PacketBuffer              Body;
        };

        typedef std::shared_ptr<Connection<TPacketEnum>> Client_t;
        typedef PacketSendInfo                           PacketInfo_t;
        typedef std::pair<Client_t, PacketInfo_t>            OwnedMessage_t;

    public:
        // Constructor: Specify Owner, connect to context, transfer the socket
        //				Provide reference to incoming message queue
        Connection(
            PacketDirection direction,
            asio::io_context& asioContext,
            asio::ip::tcp::socket socket,
            ThreadSafeQueue<OwnedMessage_t>& qIn
        )
            : m_Direction(direction),
              m_asioContext(asioContext),
              m_socket(std::move(socket)),
              m_qMessagesIn(qIn)
        {
        }

        ~Connection() = default;

    public:
        enum class ConnectionStatus : uint8_t
        {
            Disconnected = 0,
            Connecting,
            Connected
        };
    private:
        const PacketDirection m_Direction;

        ConnectionStatus m_Status = ConnectionStatus::Disconnected; //TODO Add disconnect reason

        // Each connection has a unique socket to a remote
        asio::ip::tcp::socket m_socket;

        // This context is shared with the whole asio instance
        asio::io_context& m_asioContext;

        // This queue holds all messages to be sent to the remote side
        // of this connection
        ThreadSafeQueue<PacketSendInfo> m_qMessagesOut;

        // This references the incoming queue of the parent object
        ThreadSafeQueue<OwnedMessage_t>& m_qMessagesIn;
    public:
        [[nodiscard]] inline PacketDirection  Direction()   const noexcept { return m_Direction; }
        [[nodiscard]] inline ConnectionStatus Status()      const noexcept { return m_Status; }
        [[nodiscard]] inline bool             IsConnected() const noexcept { return m_socket.is_open(); }

    public:
        void ConnectToClient();
        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints);


        void Disconnect();


        // Prime the connection to wait for incoming messages
        void StartListening()
        {
            //TODO
        }

    public:
        inline void Send(const Packet::IPacket<TPacketEnum>& packet)
        {
            PacketSendInfo info = {};
            info.Header.ID = packet.ID;

            packet.Write(info.Body);
            info.Header.Size = info.Body.size();

            //TODO Compression

            asio::post(
                m_asioContext,
                [this, &info]()
                {
                    // If the queue has a message in it, then we must assume that it is in the process of asynchronously being written.
                    // Either way add the message to the queue to be output.
                    // If no messages were available to be written, then start the process of writing the message at the front of the queue.
                    bool bWritingMessage = !m_qMessagesOut.empty();
                    m_qMessagesOut.push_back(info);
                    if (!bWritingMessage)
                    {
                        WriteHeader();
                    }
                }
            );
        }
        inline void Send(const std::unique_ptr<const Packet::IPacket<TPacketEnum>>& packet)
        {
            if(packet)
                Send(*packet);
        }

    private:
        /// ASYNC - Prime context to write a message header
        void WriteHeader();
        /// ASYNC - Prime context to write a message body
        void WriteBody();

    private:
        /// Message in middle of receiving
        PacketSendInfo m_WipInMessage = {};
    private:
        /// ASYNC - Prime context ready to read a message header
        void ReadHeader();
        /// ASYNC - Prime context ready to read a message body
        void ReadBody();

        /// Once a full message is received, add it to the incoming queue
        void AddToIncomingMessageQueue();
    };
}
namespace AWEngine::Packet::Util
{
    template<typename TPacketEnum>
    void Connection<TPacketEnum>::WriteHeader()
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

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::WriteBody()
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

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::ReadHeader()
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

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::ReadBody()
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

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::AddToIncomingMessageQueue()
    {
        switch(m_WipInMessage.Header.ID)
        {
            default:
            {
                OwnedMessage_t msg;
                if(m_Direction == PacketDirection::ToServer)
                    msg = OwnedMessage_t{ this->shared_from_this(), m_WipInMessage };
                else
                    msg = OwnedMessage_t{ nullptr, m_WipInMessage };
                m_qMessagesIn.push_back(msg);

                // We must now prime the asio context to receive the next message.
                // It wil just sit and wait for bytes to arrive, and the message construction process repeats itself.
                // Clever huh?
                ReadHeader();
                break;
            }


        }
    }

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::ConnectToClient()
    {
        if (m_Direction == PacketDirection::ToServer)
            throw std::runtime_error("Incorrect direction - Cannot connect to client from connection pointed towards server");

        if (m_socket.is_open())
        {
            ReadHeader();
        }
    }

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::ConnectToServer(const asio::ip::basic_resolver<asio::ip::tcp, asio::any_io_executor>::results_type& endpoints)
    {
        if(m_Direction == PacketDirection::ToClient)
            throw std::runtime_error("Incorrect direction - Cannot connect to server from connection pointed towards client");

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

                    std::cerr << "Failed to connect to the server: " << ec.value() << " - " << ec.message() << std::endl;
                }
            }
        );
    }

    template<typename TPacketEnum>
    void Connection<TPacketEnum>::Disconnect()
    {
        if (IsConnected())
            asio::post(m_asioContext, [this]() { m_socket.close(); });
    }
}
