#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <asio.hpp>

#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Packet/ProtocolInfo.hpp>

namespace AWEngine
{
    AWE_CLASS(PacketServer)
    {
    public:
        AWE_STRUCT(Configuration)
        {
            std::string DisplayName = "Unnamed Server";
            std::string WebsiteUrl = std::string();

            std::size_t MaxPlayers = 1;

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
    private:
        using tcp_acceptor = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::acceptor>;
        using tcp_socket = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::socket>;
    private:
        asio::awaitable<void> ListenerAsync(asio::ip::tcp tcpip);
        asio::awaitable<void> ProcessSocketAsync(tcp_socket socket);

    public:
        const Configuration Config;
    };
}
