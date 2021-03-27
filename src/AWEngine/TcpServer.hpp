#pragma once

#include <string>

#include <asio.hpp>
#include <iostream>
#include <functional>
#include <utility>

namespace AWEngine
{
    typedef std::function<bool(asio::ip::tcp::socket)> AcceptCallback_t;

    /// TCP Server/Host
    /// Has public IP and waits for connections to arrive
    AWE_CLASS(TcpServer)
    {
    public:
        static const uint16_t DefaultPort = 10101;

    public:
        explicit TcpServer(AcceptCallback_t acceptCallback, const uint16_t port = DefaultPort)
                : m_AcceptCallback(std::move(acceptCallback)),
                  m_Port(port),
                  m_Context(),
                  m_Acceptor(m_Context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
                  m_ServerPrefix("Server [" + std::to_string(m_Port) + "]")
        {
        }

    public:
        ~TcpServer()
        {
            Stop();
        }

    private:
        AcceptCallback_t m_AcceptCallback;
        const uint16_t m_Port;
    private:
        asio::io_context m_Context;
        std::thread m_ThreadContext;
        asio::ip::tcp::acceptor m_Acceptor;
    public:
        [[nodiscard]] inline const asio::io_context& Context() const noexcept { return m_Context; }

    private:
        std::string m_ServerPrefix;
    public:
        [[nodiscard]] inline const std::string& ServerPrefix() const noexcept { return m_ServerPrefix; }

    public:
        bool Start()
        {
            try
            {
                WaitForClientConnection();

                m_ThreadContext = std::thread([this]() { m_Context.run(); });
            }
            catch (std::exception& e)
            {
                std::cerr << m_ServerPrefix << ": Server start exception: " << e.what() << std::endl;
                return false;
            }

            std::cout << m_ServerPrefix << ": Server started" << std::endl;
            return true;
        }

        void Stop()
        {
            m_Context.stop();

            if (m_ThreadContext.joinable())
                m_ThreadContext.join();

#ifdef DEBUG
            std::cout << m_ServerPrefix << ": stopped." << std::endl;
#endif
        }

    private:
        /// Async
        /// Asio will wait for connection and call m_AcceptCallback before calling itself again
        void WaitForClientConnection()
        {
            m_Acceptor.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket socket)
                    {
                        // Wait for another connection...
                        // not a recursion! (calling async function)
                        WaitForClientConnection();

                        if (ec)
                        {
#ifdef DEBUG
                            std::cerr << m_ServerPrefix << ": New Connection Error: " << ec.message() << std::endl;
#endif
                            return;
                        }
                        else
                        {
                            auto remote_endpoint = socket.remote_endpoint();

#ifdef DEBUG
                            std::cout << m_ServerPrefix << ": New Connection from " << remote_endpoint << std::endl;
#endif

                            try
                            {
                                if(m_AcceptCallback(std::move(socket)))
                                {
#ifdef DEBUG
                                    std::cout << m_ServerPrefix << ": Connection from " << remote_endpoint << " approved" << std::endl;
#endif
                                    return;
                                }
                                else
                                {
#ifdef DEBUG
                                    std::cout << m_ServerPrefix << ": Connection from " << remote_endpoint << " denied" << std::endl;
#endif
                                    return;
                                }
                            }
                            catch(std::exception& ex)
                            {
#ifdef DEBUG
                                std::cout << m_ServerPrefix << ": Connection from " << remote_endpoint << " denied by exception" << std::endl;
#endif
                                return;
                            }
                        }
                    });
        }
    };
}
