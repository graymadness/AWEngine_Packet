#include "TcpClient.hpp"

#include <AWEngine/Util/DNS.hpp>

namespace AWEngine
{
    void TcpClient::AsyncConnect(const asio::ip::tcp::endpoint& endpoint)
    {
        m_Socket.async_connect(
                endpoint,
                [this](std::error_code ec)
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
    }

    TcpClient::TcpClient(const std::string& host, const uint16_t port)
            : m_Context(),
              m_Socket(asio::ip::tcp::socket(m_Context))
    {
        AsyncConnect(AWEngine::Util::DNS::ResolveHost(host, port));

        m_ContextThread = std::thread([this]() { m_Context.run(); });
    }

    TcpClient::TcpClient(const asio::ip::tcp::endpoint& endpoint)
            : m_Context(),
              m_Socket(asio::ip::tcp::socket(m_Context))
    {
        AsyncConnect(endpoint);

        m_ContextThread = std::thread([this]() { m_Context.run(); });
    }

    void TcpClient::Disconnect()
    {
        if(IsConnected())
        {
            Callback_Disconnect();
            asio::post(m_Context, [&]() { m_Socket.close(); });
        }

        m_Context.stop();

        if (m_ContextThread.joinable())
            m_ContextThread.join();
    }
}
