#pragma once

#include <string>

#include <asio.hpp>
#include <iostream>

namespace AWEngine::Packet
{
    AWE_CLASS(TcpClient)
    {
    public:
        explicit TcpClient(const std::string& host = "localhost", const uint16_t port = 10101)
                : m_Context(),
                  m_Socket(asio::ip::tcp::socket(m_Context))
        {
            asio::ip::tcp::resolver resolver(m_Context);
            auto endpoints = resolver.resolve(host, std::to_string(port));

            asio::async_connect(
                m_Socket,
                endpoints,
                [this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint)
                {
                    if(ec)
                    {
                        std::cerr << "Asio async_connect failed." << std::endl;
                        std::cerr << ec.message() << std::endl;
                    }
                    else
                    {
                        //TODO Read into callback
                    }
                }
            );

            m_ContextThread = std::thread([this]() { m_Context.run(); });
        }

    public:
        ~TcpClient()
        {
            Disconnect();
        }

    private:
        asio::io_context m_Context;
        std::thread m_ContextThread;
        asio::ip::tcp::socket m_Socket;

        /// Disconnect from server
        void Disconnect()
        {
            if(IsConnected())
            {
                //TODO Send disconnect packet
                asio::post(m_Context, [&]() { m_Socket.close(); });
            }

            m_Context.stop();

            if (m_ContextThread.joinable())
                m_ContextThread.join();
        }

        // Check if client is actually connected to a server
        [[nodiscard]] inline bool IsConnected() const { return m_Socket.is_open(); }
    };
}
