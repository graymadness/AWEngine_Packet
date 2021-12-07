#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <AWEngine/Packet/asio.hpp>
#include "IPacket.hpp"
#include "ProtocolInfo.hpp"

namespace AWEngine::Packet
{
    AWE_CLASS(PacketServer)
    {
    public:
        AWE_STRUCT(Configuration)
        {
            std::string DisplayName = "Unnamed Server";
            std::string WebsiteUrl = std::string();

            std::size_t MaxPlayers = 10;

            std::vector<asio::ip::tcp> IPs = {};
            uint16_t Port = ::AWEngine::Packet::ProtocolInfo::DefaultPort;
        };

    public:
        explicit PacketServer(Configuration  config);
        ~PacketServer();

    public:
        // Copy
        PacketServer(const PacketServer&) = delete;
        PacketServer& operator=(const PacketServer&) = delete;
        // Move
        PacketServer(PacketServer&&) = delete;
        PacketServer& operator=(PacketServer&&) = delete;

    private:
        asio::io_context m_IoContext;
        asio::ip::tcp::socket m_Socket;
#ifdef AWE_PACKET_COROUTINE
    private:
        asio::awaitable<void> ListenerAsync(asio::ip::tcp tcpip);
        asio::awaitable<void> ProcessSocketAsync(asio::tcp_socket socket);
#endif

    public:
        const Configuration Config;
    };
}
