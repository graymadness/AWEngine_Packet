#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <thread>
#include <queue>

#include <asio.hpp>

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/ToClient/Login/ServerInfo.hpp>
#include <AWEngine/Util/ThreadSafeQueue.h>

namespace AWEngine
{
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

    AWE_CLASS(PacketClient)
    {
    public:
        explicit PacketClient(std::size_t maxOutputQueueSize = MaxReceivedQueueSize_Default);
        ~PacketClient();

    public:
        // Copy
        PacketClient(const PacketClient&) = delete;
        PacketClient& operator=(const PacketClient&) = delete;
        // Move
        PacketClient(PacketClient&&) = delete;
        PacketClient& operator=(PacketClient&&) = delete;

    public:
        std::function<void(const DisconnectInfo&)> DisconnectCallback;
        std::function<void()> ConnectCallback;
    public:
        [[nodiscard]] inline bool IsConnected() const noexcept { return m_Socket.is_open(); }
        void Connect(const std::string& host, uint16_t port);
        //TODO ConnectAsync
        void Disconnect();
        //TODO DisconnectAsync

    private:
        asio::io_context m_IoContext;
        asio::ip::tcp::socket m_Socket;
    public:
        /// Callback called for every received packet (except ping and disconnect packets).
        /// Returns `true` when the packet should be put to `ReceivedQueue`.
        /// Runs on internal "Receive" thread = try to keep execution time as low as possible.
        std::function<bool(Packet::IPacket_uptr&)> PacketReceivedCallback;
        /// Queue of packets for the client to read from different threads.
        /// Try to pick items from the queue every tick otherwise it may overflow `MaxReceivedQueueSize` and terminate the connection.
        /// Cleared on `Connect`.
        ::AWEngine::Util::ThreadSafeQueue<Packet::IPacket_uptr> ReceivedQueue;
        static const std::size_t MaxReceivedQueueSize_Default = 128;
        /// Maximum items in `ReceivedQueue` until it throw an error and terminates the connection.
        const std::size_t MaxReceivedQueueSize;
        /// Option to enable/disable storing packets in `ReceivedQueue`.
        /// Not recommended to change as your only way to process packets would be inside `PacketReceivedCallback` which is not recommended.
        bool EnableReceivedQueue = true;
    public:
        template<Packet::PacketConcept_ToServer TP>
        inline void Send(const TP& packet) { ::AWEngine::Packet::IPacket::WritePacket(m_Socket, packet); }
        template<Packet::PacketConcept_ToServer TP>
        inline void Send(const std::unique_ptr<TP>& packet) { Send(*packet); }
    public:
        inline void Send(const Packet::IPacket_uptr& packet);
        //TODO SendAsync

    private:
        bool m_Closing = false;
        std::thread m_ReceiveThread = {};

    public:
        /// Retrieve information from a server.
        /// May throw an exception.
        /// Takes some time = should be run outside of main thread.
        [[nodiscard]] static ::AWEngine::Packet::ToClient::Login::ServerInfo GetServerStatus(const std::string& host, uint16_t port);
        //TODO GetServerStatusAsync
    };
}
