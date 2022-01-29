#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>
#include <utility>

#include "IPacket.hpp"
#include "ProtocolInfo.hpp"
#include "Util/Connection.hpp"

namespace AWEngine::Packet
{
    struct PacketServerConfiguration
    {
        std::string DisplayName = "Unnamed Server";
        std::string WebsiteUrl  = {};

        std::size_t MaxPlayers = 10;

        std::vector<asio::ip::tcp> IPs         = {};
        static const uint16_t      DefaultPort = 10101;
        uint16_t                   Port        = DefaultPort;
    };

    template<typename TPacketEnum>
    class PacketServer : ::AWEngine::Packet::NoCopyOrMove
    {
    public:
        static_assert(std::is_enum<TPacketEnum>());
        static_assert(sizeof(TPacketEnum) == 1);

        typedef std::function<std::shared_ptr<IPacket<TPacketEnum>>(PacketID_t, PacketBuffer&)> PacketParser_t; //THINK Convert to class?

    public:
        PacketServer(PacketServerConfiguration config, PacketParser_t packetParser)
        : m_Config(std::move(config)),
          m_Parser(std::move(packetParser))
        {
        }

        ~PacketServer()
        {
            Stop();
        }

    private:
        asio::io_context      m_IoContext;
        asio::ip::tcp::socket m_Socket;

    private:
        PacketServerConfiguration m_Config;
        PacketParser_t            m_Parser;

    public:
        bool Start();
        void Stop();
    private:
        void WaitForClientConnection();
        void MessageClient(std::shared_ptr<Util::Connection<TPacketEnum>> client, const IPacket<TPacketEnum>& msg);
        void MessageAllClients(const IPacket<TPacketEnum>& msg, std::shared_ptr<Util::Connection<TPacketEnum>> pIgnoreClient = nullptr);
    protected:
        // Called when a client connects, you can veto the connection by returning false
        virtual bool OnClientConnect(std::shared_ptr<Util::Connection<TPacketEnum>> client)
        {
            return false;
        }

        // Called when a client appears to have disconnected
        virtual void OnClientDisconnect(std::shared_ptr<Util::Connection<TPacketEnum>> client)
        {

        }

        // Called when a message arrives
        virtual void OnMessage(std::shared_ptr<Util::Connection<TPacketEnum>> client, IPacket<TPacketEnum>& msg)
        {

        }
    protected:
        typedef std::pair<std::shared_ptr<Util::Connection<TPacketEnum>>, IPacket<TPacketEnum>> OwnedMessage_t;

        // Thread Safe Queue for incoming message packets
        Util::ThreadSafeQueue<OwnedMessage_t> m_qMessagesIn;

        // Container of active validated connections
        std::deque<std::shared_ptr<Util::Connection<TPacketEnum>>> m_deqConnections;

        // Order of declaration is important - it is also the order of initialisation
        asio::io_context m_asioContext;
        std::thread m_threadContext;

        // These things need an asio context
        asio::ip::tcp::acceptor m_asioAcceptor; // Handles new incoming connection attempts...

        // Clients will be identified in the "wider system" via an ID
        uint32_t nIDCounter = 10000;
    };
}
