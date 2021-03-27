#include "TcpServer.hpp"

#include <AWEngine/Util/DNS.hpp>

namespace AWEngine
{
    [[nodiscard]] inline static std::vector<asio::ip::tcp::acceptor> CreateAcceptors(asio::io_context& context, const std::vector<asio::ip::tcp::endpoint>& endpoints)
    {
        std::vector<asio::ip::tcp::acceptor> acceptors;
        acceptors.reserve(endpoints.size());
        for(const auto& ep : endpoints)
            acceptors.emplace_back(asio::ip::tcp::acceptor(context, ep));
        return acceptors;
    }

    TcpServer::TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const std::vector<asio::ip::tcp::endpoint>& endpoints)
            : m_AcceptCallback(std::move(acceptCallback)),
              m_Endpoints(endpoints.empty() ? throw std::runtime_error("No Endpoints") : endpoints),
              m_Context(),
              m_Acceptors(CreateAcceptors(m_Context, m_Endpoints)),
              m_ServerPrefix("Server [" + name + "]")
    {
    }

    TcpServer::TcpServer(const std::string& name, AcceptCallback_t acceptCallback, const std::string& host, uint16_t port) : TcpServer(name, std::move(acceptCallback), AWEngine::Util::DNS::ResolveHosts(host, port)) {}

    bool TcpServer::Start()
    {
        if(m_Running)
        {
            std::cerr << m_ServerPrefix << " Server is already running" << std::endl;
            return false;
        }

        try
        {
            for(std::size_t i = 0; i < m_Endpoints.size(); i++)
                WaitForClientConnection(i);

            m_ThreadContext = std::thread([this]() { m_Context.run(); });
        }
        catch (std::exception& e)
        {
            std::cerr << m_ServerPrefix << " Server start exception: " << e.what() << std::endl;
            return false;
        }

        std::cout << m_ServerPrefix << " Server started" << std::endl;
        return true;
    }

    void TcpServer::WaitForClientConnection(const std::size_t acceptorIndex)
    {
        m_Acceptors[acceptorIndex].async_accept(
                [this, acceptorIndex](std::error_code ec, asio::ip::tcp::socket socket)
                {
                    if(!IsRunning())
                        return;

                    // Wait for another connection...
                    // not a recursion! (calling async function)
                    WaitForClientConnection(acceptorIndex);

                    if (ec)
                    {
#ifdef DEBUG
                        std::cerr << m_ServerPrefix << " New Connection Error: " << ec.message() << std::endl;
#endif
                        return;
                    }
                    else
                    {
                        auto remote_endpoint = socket.remote_endpoint();

#ifdef DEBUG
                        std::cout << m_ServerPrefix << " New Connection from " << remote_endpoint << std::endl;
#endif

                        try
                        {
                            if(m_AcceptCallback(std::move(socket), acceptorIndex))
                            {
#ifdef DEBUG
                                std::cout << m_ServerPrefix << " Connection from " << remote_endpoint << " approved" << std::endl;
#endif
                                return;
                            }
                            else
                            {
#ifdef DEBUG
                                std::cout << m_ServerPrefix << " Connection from " << remote_endpoint << " denied" << std::endl;
#endif
                                return;
                            }
                        }
                        catch(std::exception& ex)
                        {
#ifdef DEBUG
                            std::cout << m_ServerPrefix << " Connection from " << remote_endpoint << " denied by exception" << std::endl;
#endif
                            return;
                        }
                    }
                });
    }

    void TcpServer::Stop()
    {
        if(!m_Running)
            return;

        m_Context.stop();

        if (m_ThreadContext.joinable())
            m_ThreadContext.join();

#ifdef DEBUG
        std::cout << m_ServerPrefix << ": stopped." << std::endl;
#endif
    }
}
