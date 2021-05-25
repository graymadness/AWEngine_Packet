#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <thread>

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/ToClient/Login/ServerInfo.hpp>

namespace AWEngine::Packet
{
    AWE_ENUM(DisconnectReason, uint8_t)
    {
        Unknown         = 0,
        ClientRequest   = 1,
        Kicked          = 2,
        ConnectionError = 3
    };

    AWE_STRUCT(DisconnectInfo)
    {
        DisconnectReason Reason = DisconnectReason::Unknown;
        std::string Message = {};
    };

    class GameClient
    {
    public:
        GameClient();
        ~GameClient();

    public:
        // Copy
        GameClient(const GameClient&) = delete;
        GameClient& operator=(const GameClient&) = delete;
        // Move
        GameClient(GameClient&&) = delete;
        GameClient& operator=(GameClient&&) = delete;

    public:
        std::function<void(DisconnectInfo)> DisconnectCallback;
        std::function<void()> ConnectCallback;
    public:
        [[nodiscard]] bool IsConnected() const noexcept;
        void Connect(const std::string& host, uint16_t port);
        void Disconnect();

    public:
        std::function<void(IPacket_uptr)> PacketReceivedCallback;

        template<typename TP>
        void Send(const TP& packet);
        template<typename TP>
        inline void Send(const std::weak_ptr<TP>& packet) { Send(*packet); }

    private:
        bool m_Closing = false;
        std::thread m_NetworkThread = {};

    public:
        /// Retrieve information from a server.
        /// May throw an exception.
        /// Takes some time = should be run outside of main thread.
        static ::AWEngine::Packet::ToClient::Login::ServerInfo GetServerStatus(std::string host, uint16_t port);

    };
}
