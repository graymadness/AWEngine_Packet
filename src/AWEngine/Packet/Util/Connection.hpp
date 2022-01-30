#pragma once
#include "AWEngine/Packet/Util/Core_Packet.hpp"

#include <asio.hpp>

#include "AWEngine/Packet/IPacket.hpp"
#include "ThreadSafeQueue.hpp"
#include "AWEngine/Packet/PacketBuffer.hpp"
#include "AWEngine/Packet/Ping.hpp"

namespace AWEngine::Packet::Util
{
    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    class Connection : public std::enable_shared_from_this<Connection<TPacketID, PacketID_KeepAlive>>
    {
    public:
        struct PacketSendInfo
        {
            PacketHeader<TPacketID> Header;
            PacketBuffer              Body;
        };

        typedef std::shared_ptr<Connection<TPacketID, PacketID_KeepAlive>> Client_t;
        typedef PacketSendInfo                                             PacketInfo_t;
        typedef std::pair<Client_t, PacketInfo_t>                          OwnedMessage_t;

    public:
        /// Constructor: Specify Owner, connect to context, transfer the socket
        ///              Provide reference to incoming message queue
        Connection(
            PacketDirection                  direction,
            asio::io_context&                asioContext,
            asio::ip::tcp::socket            socket,
            ThreadSafeQueue<OwnedMessage_t>& qIn
        )
            : m_Direction(direction),
              m_IoContext(asioContext),
              m_Socket(std::move(socket)),
              m_MessagesIn(qIn)
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

        typedef std::chrono::system_clock::time_point TimePoint_t;

    private:
        const PacketDirection m_Direction;
        bool                  m_IsConnecting  = false;

        uint64_t    m_LastKeepAliveValue      = 0;
        TimePoint_t m_LastKeepAlive           = std::chrono::system_clock::now();
        TimePoint_t m_LastReceivedMessageTime = m_LastKeepAlive;
        TimePoint_t m_LastSentMessageTime     = m_LastKeepAlive;

        // Each connection has a unique socket to a remote
        asio::ip::tcp::socket m_Socket;

        // This context is shared with the whole asio instance
        asio::io_context& m_IoContext;

        // This queue holds all messages to be sent to the remote side
        // of this connection
        ThreadSafeQueue<PacketSendInfo> m_MessagesOut;

        // This references the incoming queue of the parent object
        ThreadSafeQueue<OwnedMessage_t>& m_MessagesIn;
    public:
        [[nodiscard]] inline PacketDirection  Direction()    const noexcept { return m_Direction; }
        [[nodiscard]] inline bool             IsConnecting() const noexcept { return m_IsConnecting; }
        [[nodiscard]] inline bool             IsConnected()  const noexcept { return m_Socket.is_open() && !m_IsConnecting; }
    public:
        [[nodiscard]] inline asio::ip::tcp::endpoint RemoteEndpoint() const noexcept { return IsConnected() ? m_Socket.remote_endpoint() : asio::ip::tcp::endpoint{}; }
        [[nodiscard]] inline asio::ip::tcp::endpoint LocalEndpoint()  const noexcept { return IsConnected() ? m_Socket.local_endpoint() : asio::ip::tcp::endpoint{}; }
    public:
        [[nodiscard]] inline const TimePoint_t& LastKeepAlive()           const noexcept { return m_LastKeepAlive; }
        [[nodiscard]] inline const TimePoint_t& LastReceivedMessageTime() const noexcept { return m_LastReceivedMessageTime; }
        [[nodiscard]] inline const TimePoint_t& LastSentMessageTime()     const noexcept { return m_LastSentMessageTime; }

    public:
        void ConnectToClient();
        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints);

        void Disconnect();

    public:
        inline void Send(const Packet::IPacket<TPacketID>& packet);
        inline void Send(const std::unique_ptr<Packet::IPacket<TPacketID>>& packet)
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

    // Information about number of processed packets.
    // For statistics.
    private:
        std::size_t m_ReceivedPacketCount = 0;
        std::size_t m_SentPacketCount = 0;
    public:
        [[nodiscard]] inline std::size_t ReceivedPacketCount() const noexcept { return m_ReceivedPacketCount; }
        [[nodiscard]] inline std::size_t SentPacketCount()     const noexcept { return m_SentPacketCount; }
    };
}
namespace AWEngine::Packet::Util
{
    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::WriteHeader()
    {
        if(!IsConnected())
            throw std::runtime_error("Not Connected - WriteHeader()");

        // If this function is called, we know the outgoing message queue must have at least one message to send.
        // So allocate a transmission buffer to hold the message, and issue the work - asio, send these bytes
        asio::async_write(
            m_Socket,
            asio::buffer(&m_MessagesOut.peek_front().Header, sizeof(PacketSendInfo::Header)),
            [this](std::error_code ec, std::size_t length)
            {
                // asio has now sent the bytes - if there was a problem an error would be available...
                if(!ec)
                {
                    // ... no error, so check if the message header just sent also has a message body...
                    if(!m_MessagesOut.peek_front().Body.empty())
                    {
                        // ...it does, so issue the task to write the body bytes
                        WriteBody();
                    }
                    else
                    {
                        // ...it didnt, so we are done with this message.
                        // Remove it from the outgoing message queue
                        m_MessagesOut.pop_front();

                        m_LastSentMessageTime = std::chrono::system_clock::now();
                        m_SentPacketCount++;

                        // If the queue is not empty, there are more messages to send, so make this happen by issuing the task to send the next header.
                        if(!m_MessagesOut.empty())
                            WriteHeader();
                    }
                }
                else
                {
                    // ...asio failed to write the message, we could analyse why but for now simply assume the connection has died by closing the socket.
                    // When a future attempt to write to this client fails due to the closed socket, it will be tidied up.
                    std::cerr << "Write Header Fail: " << ec.value() << " - " << ec.message() << std::endl;
                    m_Socket.close();
                }
            }
        );
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::WriteBody()
    {
        // If this function is called, a header has just been sent, and that header indicated a body existed for this message.
        // Fill a transmission buffer with the body data, and send it!
        asio::async_write(
            m_Socket,
            asio::buffer(m_MessagesOut.peek_front().Body.data(), m_MessagesOut.peek_front().Body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if(!ec)
                {
                    // Sending was successful, so we are done with the message and remove it from the queue
                    m_MessagesOut.pop_front();

                    m_LastSentMessageTime = std::chrono::system_clock::now();
                    m_SentPacketCount++;

                    // If the queue still has messages in it, then issue the task to send the next messages' header.
                    if(!m_MessagesOut.empty())
                        WriteHeader();
                }
                else
                {
                    // Sending failed, see WriteHeader() equivalent for description :P
                    std::cerr << "Write Body Fail: " << ec.value() << " - " << ec.message() << std::endl;
                    m_Socket.close();
                }
            }
        );
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::ReadHeader()
    {
        // If this function is called, we are expecting asio to wait until it receives enough bytes to form a header of a message.
        // We know the headers are a fixed size, so allocate a transmission buffer large enough to store it.
        // In fact, we will construct the message in a "temporary" message object as it's convenient to work with.
        asio::async_read(
            m_Socket,
            asio::buffer(&m_WipInMessage.Header, sizeof(PacketSendInfo::Header)),
            [this](std::error_code ec, std::size_t length)
            {
                m_WipInMessage.Header.Size = be16toh(m_WipInMessage.Header.Size); // Swap from network to local endian
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
                    std::cerr << "Read Header Fail: " << ec.value() << " - " << ec.message() << std::endl;
                    m_Socket.close();
                }
            }
        );
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::ReadBody()
    {
        m_WipInMessage.Body.resize(m_WipInMessage.Header.Size);

        // If this function is called, a header has already been read, and that header request we read a body.
        // The space for that body has already been allocated in the temporary message object, so just wait for the bytes to arrive...
        asio::async_read(
            m_Socket,
            asio::buffer(m_WipInMessage.Body.data(), m_WipInMessage.Body.size()),
            [this](std::error_code ec, std::size_t length)
            {
                if(!ec)
                {
                    if(m_WipInMessage.Header.Flags & PacketFlags::Compressed)
                    {
                        std::cerr << "Compression not implemented." << std::endl;
                        m_Socket.close();
                    }
                    else
                    {
                        // Message is complete, process it
                        AddToIncomingMessageQueue();
                    }
                }
                else
                {
                    std::cerr << "Read Body Fail: " << ec.value() << " - " << ec.message() << std::endl;
                    m_Socket.close();
                }
            }
        );
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::AddToIncomingMessageQueue()
    {
        OwnedMessage_t msg = OwnedMessage_t{ nullptr, m_WipInMessage };
        if(m_Direction == PacketDirection::ToClient) // Server's connection
            msg.first = this->shared_from_this();

        m_LastReceivedMessageTime = std::chrono::system_clock::now();
        m_ReceivedPacketCount++;

        if(msg.second.Header.ID == PacketID_KeepAlive)
        {
            if(m_Direction == PacketDirection::ToServer) // Owned by client
            {
                m_LastKeepAlive = std::chrono::system_clock::now();

                // Send back
                if(msg.second.Header.Flags == PacketFlags{} && msg.second.Header.Size == sizeof(uint64_t))
                {
                    uint64_t payload = *reinterpret_cast<const uint64_t*>(msg.second.Body.data());
                    Send(::AWEngine::Packet::Ping<TPacketID, PacketID_KeepAlive>(payload));
                }
            }

            if(m_Direction == PacketDirection::ToClient) // Owned by server
            {
                if(msg.second.Header.Flags == PacketFlags{} && msg.second.Header.Size == sizeof(uint64_t))
                {
                    uint64_t payload = *reinterpret_cast<const uint64_t*>(msg.second.Body.data());
                    if(payload == m_LastKeepAliveValue)
                        m_LastKeepAlive = std::chrono::system_clock::now();
                    else
                        m_LastKeepAlive = TimePoint_t(); // Will cause it to drop in next KeepAlive check
                }
            }
        }

        if(m_Direction == PacketDirection::ToClient) // Owned by server
        {
            //TODO Add check for too many messages waiting to be processed (denial-of-service attack)
        }

        m_MessagesIn.push_back(std::move(msg));

        // We must now prime the asio context to receive the next message.
        // It wil just sit and wait for bytes to arrive, and the message construction process repeats itself.
        // Clever huh?
        ReadHeader();
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::ConnectToClient()
    {
        if (m_Direction == PacketDirection::ToServer)
            throw std::runtime_error("Incorrect direction - Cannot connect to client from connection pointed towards server");

        if (m_Socket.is_open())
        {
            ReadHeader();
        }
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::ConnectToServer(const asio::ip::basic_resolver<asio::ip::tcp, asio::any_io_executor>::results_type& endpoints)
    {
        if(m_Direction == PacketDirection::ToClient)
            throw std::runtime_error("Incorrect direction - Cannot connect to server from connection pointed towards client");

        m_IsConnecting = true;

        // Request asio attempts to connect to an endpoint
        asio::async_connect(
            m_Socket,
            endpoints,
            [this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint)
            {
                m_IsConnecting = false;
                if (!ec)
                {
                    std::cout << "Connected to server" << std::endl;

                    ReadHeader();
                }
                else
                {
                    std::cerr << "Failed to connect to the server: " << ec.value() << " - " << ec.message() << std::endl;
                }
            }
        );
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::Disconnect()
    {
        if (IsConnected())
            asio::post(m_IoContext, [this]() { m_Socket.close(); });
    }

    template<typename TPacketID, TPacketID PacketID_KeepAlive>
    void Connection<TPacketID, PacketID_KeepAlive>::Send(const IPacket<TPacketID>& packet)
    {
        if(!IsConnected())
            throw std::runtime_error("Not Connected - Send(packet)");

        PacketSendInfo info = {};
        info.Header.ID = packet.ID;

        packet.Write(info.Body);
        info.Header.Size = htobe16(info.Body.size()); // Swap from local to network endian

        if(m_Direction == PacketDirection::ToClient)
        {
            if(info.Header.ID == PacketID_KeepAlive)
            {
                // Send back
                if(info.Header.Flags == PacketFlags{} && info.Header.Size >= sizeof(uint64_t))
                {
                    m_LastKeepAliveValue = *reinterpret_cast<const uint64_t*>(info.Body.data());
                }
            }
        }

        //TODO Compression

        asio::post(
            m_IoContext,
            [this, info]()
            {
                // If the queue has a message in it, then we must assume that it is in the process of asynchronously being written.
                // Either way add the message to the queue to be output.
                // If no messages were available to be written, then start the process of writing the message at the front of the queue.
                bool bWritingMessage = !m_MessagesOut.empty();
                m_MessagesOut.push_back(std::move(info));
                if (!bWritingMessage)
                {
                    WriteHeader();
                }
            }
        );
    }
}
