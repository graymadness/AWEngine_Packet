#pragma once

#include <string>

#include <asio.hpp>
#include <iostream>
#include <functional>
#include <utility>

namespace AWEngine
{
    typedef std::function<bool(asio::ip::tcp::socket, std::size_t)> AcceptCallback_t;

    /// TCP Server/Host
    /// Has public IP and waits for connections to arrive
    AWE_CLASS(TcpServer)
    {
    public:
        static const uint16_t DefaultPort = 10101;

    public:
        TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const std::vector<asio::ip::tcp::endpoint>& endpoint);
        inline TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const asio::ip::tcp::endpoint& endpoint)                      : TcpServer(name, std::move(acceptCallback), std::vector<asio::ip::tcp::endpoint>({endpoint})) {}
        inline TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const asio::ip::address_v4& ip4, uint16_t port = DefaultPort) : TcpServer(name, std::move(acceptCallback), asio::ip::tcp::endpoint(ip4, port)) {}
        inline TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const asio::ip::address_v6& ip6, uint16_t port = DefaultPort) : TcpServer(name, std::move(acceptCallback), asio::ip::tcp::endpoint(ip6, port)) {}
        inline TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const asio::ip::address& ip, uint16_t port = DefaultPort)     : TcpServer(name, std::move(acceptCallback), asio::ip::tcp::endpoint(ip,  port)) {}
        inline TcpServer(const std::string& name, AcceptCallback_t acceptCallback, uint16_t port = DefaultPort)                                  : TcpServer(name, std::move(acceptCallback), std::vector<asio::ip::tcp::endpoint>({ {asio::ip::tcp::v4(), port}, {asio::ip::tcp::v6(), port} })) {}
        inline TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const std::string& host, uint16_t port = DefaultPort);

    public:
        ~TcpServer()
        {
            Stop();
        }

    private:
        AcceptCallback_t m_AcceptCallback;
    public:
        [[nodiscard]] inline const AcceptCallback_t& AcceptCallback()                       const noexcept { return m_AcceptCallback; }
                      inline       void              AcceptCallback(AcceptCallback_t value)       noexcept { m_AcceptCallback = std::move(value); }
    private:
        const std::vector<asio::ip::tcp::endpoint> m_Endpoints;
        asio::io_context m_Context;
        std::thread m_ThreadContext;
        std::vector<asio::ip::tcp::acceptor> m_Acceptors;
    public:
        [[nodiscard]] inline const asio::io_context& Context() const noexcept { return m_Context; }

    private:
        std::string m_ServerPrefix;
    public:
        [[nodiscard]] inline const std::string& ServerPrefix() const noexcept { return m_ServerPrefix; }

    private:
        bool m_Running = false;
    public:
        [[nodiscard]] inline bool IsRunning() const noexcept { return m_Running; }
    public:
        bool Start();
        void Stop();

    private:
        /// Async
        /// Asio will wait for connection and call m_AcceptCallback before calling itself again
        void WaitForClientConnection(std::size_t acceptorIndex);
    };
}
