#include "PacketServer.hpp"

// Fallback to `1` when somehow not defined by CMakeLists.txt
#ifndef AWE_PACKET_SERVER_THREAD_MIN
#   define AWE_PACKET_SERVER_THREAD_MIN 1
#endif
#ifndef AWE_PACKET_SERVER_THREAD_MAX
#   define AWE_PACKET_SERVER_THREAD_MAX 1
#endif

#include <AWEngine/Packet/asio.hpp>
#include <cstdio>

namespace AWEngine::Packet
{
#ifdef AWE_PACKET_COROUTINE
    asio::awaitable<void> PacketServer::ProcessSocketAsync(asio::tcp_socket socket)
    {
        try
        {
            char data[1024];
            while(true)
            {
                std::size_t n = co_await socket.async_read_some(asio::buffer(data, 1024));
                co_await asio::async_write(socket, asio::buffer(data, n));
            }
        }
        catch (std::exception& e)
        {
            std::printf("echo Exception: %s\n", e.what());
        }
    }

    asio::awaitable<void> PacketServer::ListenerAsync(asio::ip::tcp tcpip)
    {
        auto executor = co_await asio::this_coro::executor;
        asio::tcp_acceptor acceptor(executor, { tcpip, Config.Port });
        while(true)
        {
            auto socket = co_await acceptor.async_accept();
            asio::co_spawn(executor, ProcessSocketAsync(std::move(socket)), asio::detached);
        }
    }
#endif

#ifdef AWE_PACKET_COROUTINE
    PacketServer::PacketServer(PacketServer::Configuration  config)
            : m_IoContext(std::clamp(static_cast<int>(std::thread::hardware_concurrency()), AWE_PACKET_SERVER_THREAD_MIN, AWE_PACKET_SERVER_THREAD_MAX)),
              m_Socket(m_IoContext),
              Config(std::move(config))
    {
        asio::signal_set signals(m_IoContext, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto){ m_IoContext.stop(); });

        if(Config.IPs.empty()) // Not restricted, use any
        {
            asio::co_spawn(m_IoContext, ListenerAsync(asio::ip::tcp::v4()), asio::detached);
            asio::co_spawn(m_IoContext, ListenerAsync(asio::ip::tcp::v6()), asio::detached);
        }
        else
        {
            for(const asio::ip::tcp& tcpip : Config.IPs)
                asio::co_spawn(m_IoContext, ListenerAsync(tcpip), asio::detached);
        }

        m_IoContext.run();
    }
#else
#   warning PacketServer cannot work without coroutines
#endif

    PacketServer::~PacketServer()
    {
        m_Socket.close();
    }
}
