#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <thread>
#include <queue>

#include "asio.hpp"

#include "IPacket.hpp"
#include "ProtocolInfo.hpp"
#include "AWEngine/Packet/ToClient/Login/ServerInfo.hpp"
#include "AWEngine/Packet/ToServer/Login/Init.hpp"
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

    template<
        typename TPacketID,
        TPacketID PacketID_Pong       = TPacketID(0xF0u),
        TPacketID PacketID_Disconnect = TPacketID(0xFFu),
        TPacketID PacketID_Init       = TPacketID(0xF1u)
    >
    class PacketClient : public NoCopyOrMove
    {
    public:
        static_assert(std::is_enum<TPacketID>());
        static_assert(sizeof(TPacketID) == 1);

        typedef Util::Connection<TPacketID, PacketID_Pong> Connection_t;
        typedef typename Connection_t::Connection_ptr            Client_t;
        typedef typename Connection_t::PacketInfo_t        PacketInfo_t;
        typedef std::unique_ptr<IPacket<TPacketID>>        Packet_ptr;
        typedef typename Connection_t::OwnedMessage_t      OwnedMessage_t;

        typedef std::function<Packet_ptr(PacketInfo_t&)> PacketParser_t; //THINK Convert to class?

    public:
        static const uint16_t DefaultPort = 10101;

    public:
        explicit PacketClient(
            ProtocolGameName    gameName,
            ProtocolGameVersion gameVersion
        )
            : m_GameName(gameName),
              m_GameVersion(gameVersion)
        {
            if(PacketID_Pong != TPacketID(0xF0u))
                std::cout << "Warning: Pong packet is non-standard ID" << std::endl;
            if(PacketID_Disconnect != TPacketID(0xFFu))
                std::cout << "Warning: Disconnect packet is non-standard ID" << std::endl;
        }
        ~PacketClient()
        {
            Disconnect();
        }

    private:
        PacketClientDisconnectInfo m_LastDisconnectInfo = {};
        PacketClientStatus         m_CurrentStatus      = PacketClientStatus::Disconnected;
    public:
        [[nodiscard]] inline bool                       IsConnected()        const noexcept { return m_Connection && m_Connection->IsConnected(); }
        [[nodiscard]] inline bool                       IsConnecting()       const noexcept { return m_Connection && m_Connection->IsConnecting(); }
        [[nodiscard]] inline PacketClientDisconnectInfo LastDisconnectInfo() const noexcept { return m_LastDisconnectInfo; }

    public:
        bool Connect(const std::string& host, uint16_t port = DefaultPort);
        void Disconnect();
        void DisconnectWithError();

    public:
        inline void WaitForConnect() const
        {
            while(!IsConnected() && IsConnecting())
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    private:
        asio::io_context              m_IoContext;
        std::thread                   m_ThreadContext;
        std::unique_ptr<Connection_t> m_Connection;
        asio::ip::tcp::resolver       m_Resolver = asio::ip::tcp::resolver(m_IoContext);
        ProtocolGameName              m_GameName;
        ProtocolGameVersion           m_GameVersion;
    public:
        [[nodiscard]] inline const Connection_t& Connection() const noexcept { return *m_Connection; }
        [[nodiscard]] inline       Connection_t& Connection()       noexcept { return *m_Connection; }
    public:
        inline void Send(const Packet::IPacket<TPacketID>& packet)                        { m_Connection->Send(packet); }
        inline void Send(const std::unique_ptr<const Packet::IPacket<TPacketID>>& packet) { m_Connection->Send(packet); }

    private:
        /// Queue of packets for the client to read from different threads.
        /// Try to pick items from the queue every tick otherwise it may overflow `MaxReceivedQueueSize` and terminate the connection.
        /// Cleared on `Connect`.
        Util::ThreadSafeQueue<OwnedMessage_t> ReceiveQueue;
    public:
        [[nodiscard]] inline Util::ThreadSafeQueue<OwnedMessage_t>& Incoming()    noexcept { return ReceiveQueue; }
        [[nodiscard]] inline bool                                   HasIncoming() noexcept { return !ReceiveQueue.empty(); }
    };

    inline std::ostream& operator<<(std::ostream& out, PacketClientDisconnectReason dr);
}

#include "AWEngine/Packet/ToServer/Disconnect.hpp"

namespace AWEngine::Packet
{
    template<typename TPacketID, TPacketID PacketID_Pong, TPacketID PacketID_Disconnect, TPacketID PacketID_Init>
    bool PacketClient<TPacketID, PacketID_Pong, PacketID_Disconnect, PacketID_Init>::Connect(const std::string& host, uint16_t port)
    {
        if(m_CurrentStatus != PacketClientStatus::Disconnected)
            throw std::runtime_error("Already connected");

        try
        {
            // Resolve hostname/ip-address into tangible physical address
            asio::ip::tcp::resolver::results_type endpoints = m_Resolver.resolve(host, std::to_string(port));
            if(endpoints.empty())
                throw std::runtime_error("Failed to parse/retrieve destination address");

            // Create connection
            m_Connection = std::make_unique<Connection_t>(
                PacketDirection::ToServer,
                m_IoContext,
                asio::ip::tcp::socket(m_IoContext),
                ReceiveQueue
            );

            // Tell the connection object to connect to server
            m_Connection->ConnectToServer(
                endpoints,
                std::make_unique<::AWEngine::Packet::ToServer::Login::Init<TPacketID, PacketID_Init>>(
                    m_GameName,
                    m_GameVersion,
                    Util::LocaleInfo::Current(),
                    ToServer::Login::NextInitStep::Join
                )
            );

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

    template<typename TPacketID, TPacketID PacketID_Pong, TPacketID PacketID_Disconnect, TPacketID PacketID_Init>
    void PacketClient<TPacketID, PacketID_Pong, PacketID_Disconnect, PacketID_Init>::Disconnect()
    {
        if(!IsConnected())
            return;

        // We do not know which ID is a disconnect packet...
        Send(::AWEngine::Packet::ToServer::Disconnect<TPacketID, PacketID_Disconnect>());

        // Some delay to give the Disconnect packet at least a chance to be sent to server.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

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

    template<typename TPacketID, TPacketID PacketID_Pong, TPacketID PacketID_Disconnect, TPacketID PacketID_Init>
    void PacketClient<TPacketID, PacketID_Pong, PacketID_Disconnect, PacketID_Init>::DisconnectWithError()
    {
        if(!IsConnected())
            return;

        // We do not know which ID is a disconnect packet...
        Send(::AWEngine::Packet::ToServer::Disconnect<TPacketID, PacketID_Disconnect>());

        // Some delay to give the Disconnect packet at least a chance to be sent to server.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

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
            "Requested disconnect with error"
        };
    }
}
