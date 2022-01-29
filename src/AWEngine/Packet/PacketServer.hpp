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

        asio::ip::tcp         IP          = asio::ip::tcp::v4();
        static const uint16_t DefaultPort = 10101;
        uint16_t              Port        = DefaultPort;
    };

    template<typename TPacketEnum>
    class PacketServer : public ::AWEngine::Packet::NoCopy
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
        PacketServer(PacketServerConfiguration config, PacketParser_t packetParser)
        : m_Config(std::move(config)),
          m_Parser(std::move(packetParser)),
          m_Endpoint(asio::ip::tcp::endpoint(m_Config.IP, m_Config.Port)),
          m_AsioAcceptor(m_AsioContext, m_Endpoint) //TODO Multiple IPs (at least IPv4 and IPv6 at the same time)
        {
        }

        ~PacketServer()
        {
            Stop();
        }

    private:
        PacketServerConfiguration m_Config;
        asio::ip::tcp::endpoint   m_Endpoint;
        PacketParser_t            m_Parser;

    private:
        // Thread Safe Queue for incoming message packets
        Util::ThreadSafeQueue<OwnedMessage_t> m_MessageInQueue;

        // Container of active validated connections
        std::deque<Client_t> m_Connections;

        // Order of declaration is important - it is also the order of initialisation
        asio::io_context m_AsioContext;
        std::thread      m_ThreadContext;

        // These things need an asio context
        asio::ip::tcp::acceptor m_AsioAcceptor; // Handles new incoming connection attempts...

        // Clients will be identified in the "wider system" via an ID
        uint32_t m_IDCounter = 10000;

    public:
        /// Starts the server!
        bool Start();
        /// Stops the server!
        void Stop();
    public:
        /// ASYNC - Instruct asio to wait for connection
        void WaitForClientConnection();
        /// Send a message to a specific client
        void MessageClient(const Client_t& client, const PacketInfo_t& packet);
        /// Send message to all clients
        void MessageAllClients(const PacketInfo_t& packet, const Client_t& ignoredClient = nullptr);
        /// Force server to respond to incoming messages
        void Update(size_t maximumMessages = -1, bool waitForMessage = false);
    public:
        /// Called when a client connects, you can veto the connection by returning false
        std::function<bool(const Client_t&)> OnClientConnect;
        /// Called when a client appears to have disconnected
        std::function<void(const Client_t&)> OnClientDisconnect;
        /// Called when a message arrives
        std::function<void(const Client_t&, PacketInfo_t&)> OnMessage;
    };

    template<typename TPacketEnum>
    bool PacketServer<TPacketEnum>::Start()
    {
        try
        {
            // Issue a task to the asio context - This is important as it will prime the context with "work", and stop it from exiting immediately.
            // Since this is a server, we want it primed ready to handle clients trying to connect.
            WaitForClientConnection();

            // Launch the asio context in its own thread
            m_ThreadContext = std::thread([this]() { m_AsioContext.run(); });
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

    template<typename TPacketEnum>
    void PacketServer<TPacketEnum>::Stop()
    {
        // Request the context to close
        m_AsioContext.stop();

        // Tidy up the context thread
        if (m_ThreadContext.joinable())
            m_ThreadContext.join();

        // Inform someone, anybody, if they care...
        AWE_DEBUG_COUT("Server stopped!");
    }

    template<typename TPacketEnum>
    void PacketServer<TPacketEnum>::WaitForClientConnection()
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
                    std::shared_ptr<Util::Connection<TPacketEnum>> newConnection = std::make_shared<Util::Connection<TPacketEnum>>(
                        PacketDirection::ToClient,
                        m_AsioContext,
                        std::move(socket),
                        m_MessageInQueue
                    );

                    // Give the user server a chance to deny connection
                    if (OnClientConnect && OnClientConnect(newConnection))
                    {
                        // Connection allowed, so add to container of new connections
                        m_Connections.push_back(std::move(newConnection));

                        // And very important! Issue a task to the connection's
                        // asio context to sit and wait for bytes to arrive!
                        m_Connections.back()->ConnectToClient(); // m_IDCounter++

                        AWE_DEBUG_COUT("Connection approved");
                    }
                    else
                    {
                        AWE_DEBUG_COUT("Connection Denied");

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

    template<typename TPacketEnum>
    void PacketServer<TPacketEnum>::MessageClient(const Client_t& client, const PacketInfo_t& packet)
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

    template<typename TPacketEnum>
    void PacketServer<TPacketEnum>::MessageAllClients(const PacketInfo_t& packet, const Client_t& ignoredClient)
    {
        bool bInvalidClientExists = false;

        // Iterate through all clients in container
        for (auto& client : m_Connections)
        {
            // Check client is connected...
            if (client && client->IsConnected())
            {
                // ..it is!
                if(client != ignoredClient)
                    client->Send(packet);
            }
            else
            {
                // The client couldn't be contacted, so assume it has disconnected.
                if(OnClientDisconnect)
                    OnClientDisconnect(client);

                // Off you go now, bye bye!
                client.reset();

                // Set this flag to then remove dead clients from container
                bInvalidClientExists = true;
            }
        }

        // Remove dead clients, all in one go - this way, we don't invalidate the container as we iterated through it.
        if (bInvalidClientExists)
            m_Connections.erase(std::remove(m_Connections.begin(), m_Connections.end(), nullptr), m_Connections.end());
    }

    template<typename TPacketEnum>
    void PacketServer<TPacketEnum>::Update(size_t maximumMessages, bool waitForMessage)
    {
        if (waitForMessage)
            m_MessageInQueue.wait();

        // Process as many messages as you can up to the value specified
        for(size_t messageIndex = 0; messageIndex < maximumMessages && !m_MessageInQueue.empty(); messageIndex++)
        {
            // Grab the front message
            auto msg = m_MessageInQueue.pop_front();

            // Pass to message handler
            if(OnMessage)
                OnMessage(msg.remote, msg.msg);
        }
    }
}
