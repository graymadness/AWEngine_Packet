#pragma once
#include "AWEngine/Packet/Util/Core_Packet.hpp"

#include <asio.hpp>

#include "AWEngine/Packet/IPacket.hpp"
#include "ThreadSafeQueue.hpp"
#include "AWEngine/Packet/PacketBuffer.hpp"

namespace AWEngine::Packet::Util
{
    struct PacketSendInfo
    {
        PacketHeader Header;
        PacketBuffer Body;
    };

    class Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        // Constructor: Specify Owner, connect to context, transfer the socket
        //				Provide reference to incoming message queue
        Connection(
            PacketDirection direction,
            asio::io_context& asioContext,
            asio::ip::tcp::socket socket,
            ThreadSafeQueue<PacketSendInfo>& qIn
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
        ThreadSafeQueue<PacketSendInfo>& m_qMessagesIn;
    public:
        [[nodiscard]] inline PacketDirection  Direction()   const noexcept { return m_Direction; }
        [[nodiscard]] inline ConnectionStatus Status()      const noexcept { return m_Status; }
        [[nodiscard]] inline bool             IsConnected() const noexcept { return m_socket.is_open(); }

    public:
        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints);


        void Disconnect();


        // Prime the connection to wait for incoming messages
        void StartListening()
        {
            //TODO
        }

    public:
        template<Packet::PacketConcept_ToServer TP>
        inline void Send(const TP& packet)
        {
            PacketSendInfo info = {};
            info.Header.ID = TP::s_PacketID();
            packet.Write(info.Body);
            info.Header.Size = info.Body.size();

            asio::post(
                m_asioContext,
                [this, &info]()
                {
                    //TODO Compression

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
        template<Packet::PacketConcept_ToServer TP>
        inline void Send(const std::unique_ptr<TP>& packet) { Send(*packet); }

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
