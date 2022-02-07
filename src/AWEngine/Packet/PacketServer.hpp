#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>
#include <utility>

#include "IPacket.hpp"
#include "ProtocolInfo.hpp"
#include "Util/Connection.hpp"
#include "ProtocolGameName.hpp"

namespace AWEngine::Packet
{
    struct PacketServerConfiguration
    {
        std::string DisplayName = "Unnamed Server";
        std::string WebsiteUrl  = {};

        std::size_t MaxPlayers = 10;

        asio::ip::tcp         IP          = asio::ip::tcp::v4();
        static const uint16_t DefaultPort = 10101;
        uint16_t              Port        = DefaultPort;
    };

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping = TPacketID(0xF0u),
        TPacketID PacketID_Kick = TPacketID(0xFFu),
        TPacketID PacketID_ServerInfo = TPacketID(0xF1u)
    >
    class PacketServer : public ::AWEngine::Packet::NoCopy
    {
    public:
        static_assert(std::is_enum<TPacketID>());
        static_assert(sizeof(TPacketID) == 1);

        typedef Util::Connection<TPacketID, PacketID_Ping, PacketID_ServerInfo> Connection_t;
        typedef typename Connection_t::Connection_ptr                           Connection_ptr;
        typedef std::unique_ptr<IPacket<TPacketID>>                             Packet_ptr;
        typedef std::unique_ptr<IPacket<TPacketID_Receive>>                     Packet_Receive_ptr;
        typedef typename Connection_t::OwnedMessage_t                           OwnedMessage_t;

        typedef std::function<Packet_Receive_ptr(Util::PacketSendInfo&)> PacketParser_t; //THINK Convert to class?

    public:
        PacketServer(
            PacketServerConfiguration config,
            PacketParser_t            packetParser,
            ProtocolGameName          gameName,
            ProtocolGameVersion       gameVersion
        )
            : m_Config(std::move(config)),
              m_Parser(std::move(packetParser)),
              m_Endpoint(asio::ip::tcp::endpoint(m_Config.IP, m_Config.Port)),
              m_AsioAcceptor(m_IoContext, m_Endpoint), //TODO Multiple IPs (at least IPv4 and IPv6 at the same time)
              m_GameName(gameName),
              m_GameVersion(gameVersion)
        {
            if(!m_Parser)
                throw std::runtime_error("No parser provided");

            if(PacketID_Ping != TPacketID(0xF0u))
                std::cerr << "Warning: Ping packet has non-standard ID" << std::endl;
            if(PacketID_Kick != TPacketID(0xFFu))
                std::cerr << "Warning: Kick packet has non-standard ID" << std::endl;
            if(PacketID_ServerInfo != TPacketID(0xF1u))
                std::cerr << "Warning: ServerInfo packet has non-standard ID" << std::endl;
        }

        ~PacketServer()
        {
            Stop();
        }

    private:
        PacketServerConfiguration m_Config;
        asio::ip::tcp::endpoint   m_Endpoint;
        PacketParser_t            m_Parser;
        ProtocolGameName          m_GameName;
        ProtocolGameVersion       m_GameVersion;

    public:
        [[nodiscard]] inline Packet_ptr ParsePacket(Util::PacketSendInfo& info) { return m_Parser(info); }

    private:
        // Thread Safe Queue for incoming message packets
        Util::ThreadSafeQueue<OwnedMessage_t> m_MessageInQueue;

        // Container of active validated connections
        std::deque<Connection_ptr> m_Connections;

        // Order of declaration is important - it is also the order of initialisation
        asio::io_context m_IoContext;
        std::thread      m_ThreadContext;

        // These things need an asio context
        asio::ip::tcp::acceptor m_AsioAcceptor; // Handles new incoming connection attempts...

        std::chrono::system_clock::duration KeepAliveDelay    = std::chrono::seconds(30);
        std::chrono::system_clock::duration KeepAliveMaxDelay = std::chrono::seconds(45);

    public:
        /// Starts the server!
        bool Start();
        /// Stops the server!
        void Stop();
    private:
        /// ASYNC - Instruct asio to wait for connection
        void WaitForClientConnection();
    public:
        /// Send a message to a specific client.
        void Send(const Connection_ptr& client, const IPacket<TPacketID>& packet);
        /// Send packet to all clients.
        void Send_AllClients(const IPacket<TPacketID>& packet, const Connection_ptr& ignoredClient = nullptr);
        /// Force server to respond to incoming messages.
        std::size_t Update(size_t maximumMessages = -1, bool waitForMessage = false);
        /// Sends KeepAlive packet to all clients.
        void SendKeepAlive();

    public:
        /// Called when a client connects, you can veto the connection by returning false.
        /// Use to ban based on IP address (based on local cache).
        /// Called before receiving intent - join or server info.
        /// Leave empty to accept all
        std::function<bool(const Connection_ptr&)> OnClientConnect;
        /// Called after receiving client's intent to join.
        std::function<void(const Connection_ptr&)> OnClientJoin;
        /// Called after receiving client's intent to retrieve info.
        std::function<std::string(const Connection_ptr&)> OnClientServerInfoRequest;
        /// Called when a client appears to have disconnected.
        std::function<void(const Connection_ptr&)> OnClientDisconnect;
        /// Called when a message arrives.
        /// Processed during `Update` not when received.
        /// Warning: Header flags are original (unprocessed) but the packet was already processed from network format.
        std::function<void(const Connection_ptr&, Util::PacketSendInfo&)> OnMessage;
        /// Called when a message arrives and is processed into a packet.
        /// Processed during `Update` not when received.
        std::function<void(const Connection_ptr&, Packet_ptr&)> OnPacket;
    };
}

namespace AWEngine::Packet
{
    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    bool PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::Start()
    {
        try
        {
            // Issue a task to the asio context - This is important as it will prime the context with "work", and stop it from exiting immediately.
            // Since this is a server, we want it primed ready to handle clients trying to connect.
            WaitForClientConnection();

            // Launch the asio context in its own thread
            m_ThreadContext = std::thread([this]() { m_IoContext.run(); });
        }
        catch (std::exception& ex)
        {
            // Something prohibited the server from listening
            std::cerr << "Server exception: " << ex.what() << std::endl;
            DEBUG_BREAK
            return false;
        }

        AWE_DEBUG_COUT("Server started!");
        return true;
    }

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    void PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::Stop()
    {
        // Request the context to close
        m_IoContext.stop();

        // Tidy up the context thread
        if (m_ThreadContext.joinable())
            m_ThreadContext.join();

        // Inform someone, anybody, if they care...
        AWE_DEBUG_COUT("Server stopped!");
    }

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    void PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::WaitForClientConnection()
    {
        // Prime context with an instruction to wait until a socket connects.
        // This is the purpose of an "acceptor" object.
        // It will provide a unique socket for each incoming connection attempt.
        m_AsioAcceptor.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket socket)
            {
                // Triggered by incoming connection request
                if (!ec)
                {
                    // Display some useful(?) information
                    AWE_DEBUG_COUT("New connection from " << socket.remote_endpoint());

                    // Create a new connection to handle this client
                    std::shared_ptr<Connection_t> newConnection = std::make_shared<Connection_t>(
                        PacketDirection::ToClient,
                        m_IoContext,
                        std::move(socket),
                        m_MessageInQueue
                    );

                    // Give the user server a chance to deny connection
                    if (!OnClientConnect || OnClientConnect(newConnection))
                    {
                        // Connection allowed, so add to container of new connections
                        m_Connections.push_back(newConnection);

                        // And very important!
                        // Issue a task to the connection's asio context to sit and wait for bytes to arrive!
                        newConnection->ConnectToClient();

                        AWE_DEBUG_COUT("Connection from " << newConnection->RemoteEndpoint() << " approved");
                    }
                    else
                    {
                        AWE_DEBUG_COUT("Connection from " << newConnection->RemoteEndpoint() << " denied");

                        // Connection will go out of scope with no pending tasks, so will get destroyed automagically due to the wonder of smart pointers
                    }
                }
                else
                {
                    // Error has occurred during acceptance
                    std::cout << "Server - new connection error: " << ec.message() << "\n";
                }

                // Prime the asio context with more work - again simply wait for another connection...
                WaitForClientConnection();
            }
        );
    }

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    void PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::Send(const Connection_ptr& client, const IPacket<TPacketID>& packet)
    {
        // Check client is legitimate...
        if (client && client->IsConnected())
        {
            // ...and post the message via the connection
            client->Send(packet);
        }
        else
        {
            // If we can't communicate with client then we may as well remove the client - let the server know, it may be tracking it somehow
            if(OnClientDisconnect)
                OnClientDisconnect(client);

            // Off you go now, bye bye!
            client.reset();

            // Then physically remove it from the container
            m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), client), m_Connections.end());
        }
    }

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    void PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::Send_AllClients(const IPacket<TPacketID>& packet, const Connection_ptr& ignoredClient)
    {
        bool invalidClientExists = false;

        // Iterate through all clients in container
        for(auto& connection : m_Connections)
        {
            // Check client is connected...
            if(connection && connection->IsConnected())
            {
                // ..it is!
                if(connection != ignoredClient)
                    connection->Send(packet);
            }
            else if(connection && connection->IsConnecting())
            {
                // Nothing when connecting
            }
            else
            {
                // The client couldn't be contacted, so assume it has disconnected.
                if(OnClientDisconnect)
                    OnClientDisconnect(connection);

                // Off you go now, bye bye!
                connection.reset();

                // Set this flag to then remove dead clients from container
                invalidClientExists = true;
            }
        }

        // Remove dead clients, all in one go - this way, we don't invalidate the container as we iterated through it.
        if (invalidClientExists)
            m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), nullptr), m_Connections.end());
    }

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    std::size_t PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::Update(size_t maximumMessages, bool waitForMessage)
    {
        if (waitForMessage)
            m_MessageInQueue.wait();

        if(maximumMessages == 0)
            return 0;

        // Process as many messages as you can up to the value specified
        size_t messageIndex;
        for(messageIndex = 0; !m_MessageInQueue.empty() && (maximumMessages < 0 || messageIndex < maximumMessages); messageIndex++)
        {
            // Grab the front message
            auto msg = m_MessageInQueue.pop_front();

            if(msg.second.Header.Flags & PacketFlags::Compressed)
            {
                throw std::runtime_error("Packet compression is not supported");
            }

            // Pass to message handler
            if(OnMessage)
                OnMessage(msg.first, msg.second);

            //TODO Process some universal packets if their IDs were defined

            std::unique_ptr<IPacket<TPacketID>> packet = ParsePacket(msg.second);
            if(packet)
                if(OnPacket)
                    OnPacket(msg.first, packet);
        }
        return messageIndex;
    }

    template<
        typename TPacketID,
        typename TPacketID_Receive,
        TPacketID PacketID_Ping,
        TPacketID PacketID_Kick,
        TPacketID PacketID_ServerInfo
    >
    void PacketServer<TPacketID, TPacketID_Receive, PacketID_Ping, PacketID_Kick, PacketID_ServerInfo>::SendKeepAlive()
    {
        auto oldestKeepAlive = std::chrono::system_clock::now() - KeepAliveMaxDelay;

        bool invalidClientExists = false;
        for(auto& connection : m_Connections)
        {
            // Check client is connected...
            if(!connection || (!connection->IsConnected() && !connection->IsConnecting()) || (connection->LastKeepAlive() < oldestKeepAlive))
            {
                // The client couldn't be contacted, so assume it has disconnected.
                if(OnClientDisconnect)
                    OnClientDisconnect(connection);

                // Off you go now, bye bye!
                connection.reset();

                // Set this flag to then remove dead clients from container
                invalidClientExists = true;
            }
        }

        if(invalidClientExists)
            m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), nullptr), m_Connections.end());
        invalidClientExists = false;

        //TODO Only send KeepAlive to clients with last KeepAlive packet between KeepAliveDelay and KeepAliveMaxDelay
        Ping<TPacketID, PacketID_Ping> ping = Ping<TPacketID, PacketID_Ping>();
        Send_AllClients(ping);
    }
}
