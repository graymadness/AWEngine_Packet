#pragma once

#include <asio.hpp>
#ifdef AWE_PACKET_COROUTINE
#   include <coroutine>
#endif

namespace asio
{
#ifdef AWE_PACKET_COROUTINE
    using tcp_acceptor = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::acceptor>;
    using tcp_socket = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::socket>;
#endif
}
