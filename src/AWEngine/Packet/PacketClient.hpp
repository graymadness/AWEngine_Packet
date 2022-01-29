#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <thread>
#include <queue>

#include "asio.hpp"

#include "IPacket.hpp"
#include "PacketWrapper.hpp"
#include "ProtocolInfo.hpp"
#include "AWEngine/Packet/ToClient/Login/ServerInfo.hpp"
#include <AWEngine/Packet/Util/ThreadSafeQueue.h>

namespace AWEngine::Packet
{
    AWE_CLASS(PacketClient)
    {
    public:
        AWE_ENUM(Status, uint8_t)
        {
            Disconnected = 0,
            Connecting,
            Connected
        };

        AWE_ENUM(DisconnectReason, uint8_t)
        {
            Unknown = 0,
            ClientRequest = 1,
            Kicked = 2,
            ConnectionError = 3
        };

        AWE_STRUCT(DisconnectInfo)
        {
            DisconnectReason Reason = DisconnectReason::Unknown;
            bool TranslateMessage = false;
            std::string Message = {};
        };

    public:
        explicit PacketClient(
            std::size_t maxReceiveQueueSize = MaxReceiveQueueSize_Default
                );
        ~PacketClient();

    public:
        // Copy
        PacketClient(const PacketClient&) = delete;
        PacketClient& operator=(const PacketClient&) = delete;
        // Move
        PacketClient(PacketClient&&) = delete;
        PacketClient& operator=(PacketClient&&) = delete;

    private:
        DisconnectInfo m_LastDisconnectInfo = {};
        Status         m_CurrentStatus = Status::Disconnected;
    public:
        [[nodiscard]] inline bool           IsConnected()        const noexcept { return m_Socket.is_open(); }
        [[nodiscard]] inline Status         CurrentStatus()      const noexcept { return m_CurrentStatus; }
        [[nodiscard]] inline DisconnectInfo LastDisconnectInfo() const noexcept { return m_LastDisconnectInfo; }

    public:
        void Connect(const std::string& host, uint16_t port = ::AWEngine::Packet::ProtocolInfo::DefaultPort);
        void Disconnect();

    private:
        asio::io_context        m_IoContext;
        asio::ip::tcp::endpoint m_EndPoint;
        asio::ip::tcp::socket   m_Socket;
        std::thread             m_ThreadContext;
    public:
        /// Queue of packets for the client to read from different threads.
        /// Try to pick items from the queue every tick otherwise it may overflow `MaxReceivedQueueSize` and terminate the connection.
        /// Cleared on `Connect`.
        ::AWEngine::Util::ThreadSafeQueue<Packet::IPacket_uptr> ReceiveQueue;
        static const std::size_t MaxReceiveQueueSize_Default = 128;
        /// Maximum items in `ReceivedQueue` until it throw an error and terminates the connection.
        const std::size_t MaxReceiveQueueSize;
    private:
        struct SendInfo
        {
            PacketHeader Header;
            PacketBuffer Body;
        };
        ::AWEngine::Util::ThreadSafeQueue<SendInfo> m_SendQueue;
    private:
        void SendAsync_Header();
        void SendAsync_Body(PacketBuffer);
    public:
        template<Packet::PacketConcept_ToServer TP>
        inline void Send(const TP& packet);
        template<Packet::PacketConcept_ToServer TP>
        inline void Send(const std::unique_ptr<TP>& packet) { Send(*packet); }
    public:
        inline void Send(const Packet::IPacket_uptr& packet);
        //TODO SendAsync

    private:
        bool m_Closing = false;

    // Stats
    private:
        std::size_t m_SentPacketCount = 0;
        std::size_t m_ReceivedPacketCount = 0;
    public:
        [[nodiscard]] inline std::size_t SentPacketCount()     const noexcept { return m_SentPacketCount; }
        [[nodiscard]] inline std::size_t ReceivedPacketCount() const noexcept { return m_ReceivedPacketCount; }

    public:
        /// Retrieve information from a server.
        /// May throw an exception.
        /// Takes some time = should be run outside of main thread.
        [[nodiscard]] static ::AWEngine::Packet::ToClient::Login::ServerInfo GetServerStatus(const std::string& host, uint16_t port);
        [[nodiscard]] static void GetServerStatusAsync(const std::string& host, uint16_t port, std::function<::AWEngine::Packet::ToClient::Login::ServerInfo> infoCallback, std::function<void> errorcallback);
    };

    inline std::ostream& operator<<(std::ostream& out, PacketClient::DisconnectReason dr);
}

namespace AWEngine::Packet
{
    // ASYNC - Prime context to write a message header
    void PacketClient::SendAsync_Header()
    {
        auto info = m_SendQueue.pop_front();

        // If this function is called, we know the outgoing message queue must have
        // at least one message to send. So allocate a transmission buffer to hold
        // the message, and issue the work - asio, send these bytes
        asio::async_write(
            m_Socket,
            asio::buffer(&info.Header, sizeof(PacketHeader)),
            [this, &info](std::error_code ec, std::size_t length)
            {
                // asio has now sent the bytes - if there was a problem
                // an error would be available...
                if(!ec)
                {
                    m_SentPacketCount++;

                    if(!info.Body.empty())
                    {
                        // ...it does, so issue the task to write the body bytes
                        SendAsync_Body(std::move(info.Body));
                    }
                    else
                    {
                        // If the queue is not empty, there are more messages to send, so
                        // make this happen by issuing the task to send the next header.
                        if(!m_SendQueue.empty())
                        {
                            SendAsync_Header();
                        }
                    }
                }
                else
                {
                    // ...asio failed to write the message, we could analyse why but
                    // for now simply assume the connection has died by closing the
                    // socket. When a future attempt to write to this client fails due
                    // to the closed socket, it will be tidied up.
                    m_Socket.close();

                    m_CurrentStatus = Status::Disconnected;
                    m_LastDisconnectInfo = {
                        DisconnectReason::ConnectionError,
                        false,
                        "Write Header Fail"
                    };
                }
            }
        );
    }

    // ASYNC - Prime context to write a message body
    void PacketClient::SendAsync_Body(PacketBuffer buffer)
    {
        //TODO Will `buffer` survive?

        // If this function is called, a header has just been sent, and that header
        // indicated a body existed for this message. Fill a transmission buffer
        // with the body data, and send it!
        asio::async_write(
            m_Socket,
            asio::buffer(buffer.data(), buffer.size()),
            [this, &buffer](std::error_code ec, std::size_t length)
            {
                if(!ec)
                {
                    // If the queue still has messages in it, then issue the task to
                    // send the next messages' header.
                    if(!m_SendQueue.empty())
                    {
                        SendAsync_Header();
                    }
                }
                else
                {
                    // Sending failed, see SendAsync_Header() equivalent for description :P
                    m_Socket.close();

                    m_CurrentStatus = Status::Disconnected;
                    m_LastDisconnectInfo = {
                        DisconnectReason::ConnectionError,
                        false,
                        "Write Header Fail - " + std::to_string(buffer.size()) + " bytes"
                    };
                }
            }
        );
    }

    template<Packet::PacketConcept_ToServer TP>
    void PacketClient::Send(const TP& packet)
    {
        SendInfo info = {
            PacketHeader{
                TP::s_PacketID(),
                0,
                {}
            },
            PacketWrapper::WritePacket(packet)
        };
        info.Header.Size = info.Body.size();

        asio::post(
            m_IoContext,
            [this, info]()
            {
                // If the queue has a message in it, then we must
                // assume that it is in the process of asynchronously being written.
                // Either way add the message to the queue to be output. If no messages
                // were available to be written, then start the process of writing the
                // message at the front of the queue.
                bool bWritingMessage = !m_SendQueue.empty();
                m_SendQueue.push_back(info);
                if(!bWritingMessage)
                {
                    SendAsync_Header();
               }
           }
       );
    }

    std::ostream& operator<<(std::ostream& out, PacketClient::DisconnectReason dr)
    {
        switch(dr)
        {
            default:
            case PacketClient::DisconnectReason::Unknown:
                return out << "unknown";
            case PacketClient::DisconnectReason::ClientRequest:
                return out << "client_request";
            case PacketClient::DisconnectReason::Kicked:
                return out << "kicked";
            case PacketClient::DisconnectReason::ConnectionError:
                return out << "connection_error";
        }
    }
}
