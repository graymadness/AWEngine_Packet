#include <cassert>

#include "AWEngine/Packet/PacketServer.hpp"
#include "AWEngine/Packet/PacketClient.hpp"

void StartServer();
void StartClient();

int main(int argc, const char** argv)
{
    std::thread t_server = std::thread(StartServer);
    std::thread t_client = std::thread(StartClient);

    t_client.join();
    t_server.join();
}

void StartServer()
{
    using namespace AWEngine;
    using namespace AWEngine::Packet;

    PacketServer::Configuration serverConfig;
    serverConfig.DisplayName = "Code Test Server";
    serverConfig.MaxPlayers = 1;
    PacketServer server(serverConfig);
}

void StartClient()
{
    using namespace AWEngine;
    using namespace AWEngine::Packet;

    PacketClient client = PacketClient();
    client.ConnectCallback = []
    {
        std::cout << "Client: " << "Connect" << std::endl;
    };
    client.DisconnectCallback = [](const AWEngine::DisconnectInfo& disconnectInfo)
    {
        std::cout << "Client: " << "Disconnect, " << disconnectInfo.Reason << disconnectInfo.Message << std::endl;
    };
    client.PacketReceivedCallback = [](Packet::IPacket_uptr& packet) -> bool
    {
        std::cout << "Client: " << "Packet received" << std::endl;
    };
    client.EnableReceivedQueue = false; // Do not use receive queue as it is not cleared
    client.Connect("127.0.0.1");
}
