#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <thread>
#include <queue>

#include "asio.hpp"

#include "IPacket.hpp"
#include "ProtocolInfo.hpp"
#include "AWEngine/Packet/ToClient/Login/ServerInfo.hpp"
#include "AWEngine/Packet/Util/ThreadSafeQueue.hpp"
#include "AWEngine/Packet/Util/Connection.hpp"

namespace AWEngine::Packet
{
    enum class PacketClientStatus : uint8_t
    {
        Disconnected = 0,
        Connecting,
        Connected
    };

    enum class PacketClientDisconnectReason : uint8_t
    {
        Unknown = 0,
        ClientRequest = 1,
        Kicked = 2,
        ConnectionError = 3
    };

    struct PacketClientDisconnectInfo
    {
        PacketClientDisconnectReason Reason = PacketClientDisconnectReason::Unknown;
        bool TranslateMessage = false;
        std::string Message = {};
    };

    template<typename TPacketEnum>
    class PacketClient : public NoCopyOrMove
    {
    public:
        static_assert(std::is_enum<TPacketEnum>());
        static_assert(sizeof(TPacketEnum) == 1);

        typedef typename Util::Connection<TPacketEnum>::Client_t       Client_t;
        typedef typename Util::Connection<TPacketEnum>::PacketInfo_t   PacketInfo_t;
        typedef std::unique_ptr<IPacket<TPacketEnum>>                  Packet_ptr;
        typedef typename Util::Connection<TPacketEnum>::OwnedMessage_t OwnedMessage_t;

        typedef std::function<Packet_ptr(PacketInfo_t&)> PacketParser_t; //THINK Convert to class?

    public:
        static const uint16_t DefaultPort = 10101;

    public:
        explicit PacketClient() = default;
        ~PacketClient()
        {
            Disconnect();
        }

    private:
        PacketClientDisconnectInfo m_LastDisconnectInfo = {};
        PacketClientStatus         m_CurrentStatus      = PacketClientStatus::Disconnected;
    public:
        [[nodiscard]] inline bool                       IsConnected()        const noexcept { return m_Connection && m_Connection->IsConnected(); }
        [[nodiscard]] inline PacketClientDisconnectInfo LastDisconnectInfo() const noexcept { return m_LastDisconnectInfo; }

    public:
        bool Connect(const std::string& host, uint16_t port = DefaultPort);
        template<TPacketEnum disconnectPacketId>
        void Disconnect();
        void Disconnect();

    private:
        asio::io_context                               m_IoContext;
        asio::ip::tcp::endpoint                        m_EndPoint;
        std::thread                                    m_ThreadContext;
        std::unique_ptr<Util::Connection<TPacketEnum>> m_Connection;
    public:
        inline void Send(const Packet::IPacket<TPacketEnum>& packet)                        { m_Connection->Send(packet); }
        inline void Send(const std::unique_ptr<const Packet::IPacket<TPacketEnum>>& packet) { m_Connection->Send(packet); }

    private:
        /// Queue of packets for the client to read from different threads.
        /// Try to pick items from the queue every tick otherwise it may overflow `MaxReceivedQueueSize` and terminate the connection.
        /// Cleared on `Connect`.
        Util::ThreadSafeQueue<OwnedMessage_t> ReceiveQueue;
    public:
        [[nodiscard]] inline Util::ThreadSafeQueue<OwnedMessage_t>& Incoming()          noexcept { return ReceiveQueue; }
        [[nodiscard]] inline bool                                   HasIncoming() const noexcept { return !ReceiveQueue.empty(); }
    };

    inline std::ostream& operator<<(std::ostream& out, PacketClientDisconnectReason dr);
}

#include "AWEngine/Packet/Util/DNS.hpp"
#include "AWEngine/Packet/ToServer/Disconnect.hpp"

namespace AWEngine::Packet
{
    template<typename TPacketEnum>
    bool PacketClient<TPacketEnum>::Connect(const std::string& host, uint16_t port)
    {
        if(m_CurrentStatus != PacketClientStatus::Disconnected)
            throw std::runtime_error("Already connected");

        try
        {
            // Resolve hostname/ip-address into tangiable physical address
            asio::ip::tcp::resolver resolver(m_IoContext);
            asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

            // Create connection
            m_Connection = std::make_unique<Util::Connection<TPacketEnum>>(PacketDirection::ToServer, m_IoContext, asio::ip::tcp::socket(m_IoContext), ReceiveQueue);

            // Tell the connection object to connect to server
            m_Connection->ConnectToServer(endpoints);

            // Start Context Thread
            m_ThreadContext = std::thread([this]() { m_IoContext.run(); });
        }
        catch (std::exception& e)
        {
            std::cerr << "Client Exception: " << e.what() << std::endl;

            m_LastDisconnectInfo = {
                PacketClientDisconnectReason::ConnectionError,
                false,
                "Failed to connect"
            };
            return false;
        }

        return true;
    }

    template<typename TPacketEnum>
    template<TPacketEnum disconnectPacketId>
    void PacketClient<TPacketEnum>::Disconnect()
    {
        if(!IsConnected())
            return;

        // Tell server that the disconnect was "by decision" and not an error
        Send(::AWEngine::Packet::ToServer::Disconnect<TPacketEnum, disconnectPacketId>());

        // Either way, we're also done with the asio context...
        m_IoContext.stop();
        // ...and its thread
        if (m_ThreadContext.joinable())
            m_ThreadContext.join();

        // Destroy the connection object
        m_Connection.release();

        m_LastDisconnectInfo = {
            PacketClientDisconnectReason::ClientRequest,
            false,
            "Disconnect requested by user"
        };
    }

    template<typename TPacketEnum>
    void PacketClient<TPacketEnum>::Disconnect()
    {
        if(!IsConnected())
            return;

        // We do not know which ID is a disconnect packet...
        //Send(::AWEngine::Packet::ToServer::Disconnect<TPacketEnum, disconnectPacketId>());

        // Either way, we're also done with the asio context...
        m_IoContext.stop();
        // ...and its thread
        if (m_ThreadContext.joinable())
            m_ThreadContext.join();

        // Destroy the connection object
        m_Connection.release();

        m_LastDisconnectInfo = {
            PacketClientDisconnectReason::ClientRequest,
            false,
            "Disconnect requested by user without Disconnect packet"
        };
    }
}
