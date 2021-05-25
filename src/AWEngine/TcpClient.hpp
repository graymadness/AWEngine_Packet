#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <string>
#include <iostream>

#include <asio.hpp>

#include <AWEngine/Packet/ProtocolInfo.hpp>

namespace AWEngine
{
    /// Client which connects to Server using TCP
    AWE_CLASS(TcpClient)
    {
    public:
        explicit TcpClient(const std::string& host = "localhost", uint16_t port = ::AWEngine::Packet::ProtocolInfo::DefaultPort);
        explicit TcpClient(const asio::ip::tcp::endpoint& endpoint);
        inline explicit TcpClient(const asio::ip::address& address, uint16_t port = ::AWEngine::Packet::ProtocolInfo::DefaultPort) : TcpClient(asio::ip::tcp::endpoint(address, port)) {}

    public:
        ~TcpClient()
        {
            Disconnect();
        }

    private:
        void AsyncConnect(const asio::ip::tcp::endpoint& endpoint);

    public:
        std::function<void()> Callback_Disconnect;

    private:
        asio::io_context m_Context;
        std::thread m_ContextThread;
        asio::ip::tcp::socket m_Socket;

        /// Disconnect from server
        void Disconnect();

        // Check if client is actually connected to a server
        [[nodiscard]] inline bool IsConnected() const { return m_Socket.is_open(); }
    };
}
