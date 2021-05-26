#include <AWEngine/PacketServer.hpp>

#include <cassert>

int main(int argc, const char** argv)
{
    using namespace AWEngine;
    using namespace AWEngine::Packet;

    PacketServer::Configuration serverConfig;
    PacketServer server(serverConfig);
}
