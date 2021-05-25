#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include "TcpClient.hpp"

namespace AWEngine
{
    class IPacket;

    typedef std::function<void(IPacket packet)> PacketCallback;

    class PacketClient
    {
    public:
        explicit PacketClient(PacketCallback& callback, const std::string& host = "localhost", uint16_t port = 10101);
    };
}
