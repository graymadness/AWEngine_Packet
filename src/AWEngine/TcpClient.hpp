#pragma once

#include <string>

#include <asio.hpp>
#include <iostream>

namespace AWEngine
{
    /// Client which connects to Server using TCP
    AWE_CLASS(TcpClient)
    {
    public:
        static const uint16_t DefaultPort = 10101;

    public:
        explicit TcpClient(const std::string& host = "localhost", uint16_t port = DefaultPort);
        explicit TcpClient(const asio::ip::tcp::endpoint& endpoint);
        inline  TcpClient(const asio::ip::address& address, uint16_t port = DefaultPort) : TcpClient(asio::ip::tcp::endpoint(address, port)) {}

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
