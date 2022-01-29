#include <cassert>

#include "AWEngine/Packet/PacketServer.hpp"
#include "AWEngine/Packet/PacketClient.hpp"

enum class PacketID : uint8_t
{
    MsgA = 0u,
    MsgB = 1u
};

using namespace AWEngine;
using namespace AWEngine::Packet;

struct MsgA : public IPacket<PacketID>
{
    MsgA() : IPacket<PacketID>(PacketID::MsgA) {}

    void Write(PacketBuffer& out) const override {}
};

struct MsgB : public IPacket<PacketID>
{
    MsgB() : IPacket<PacketID>(PacketID::MsgB) {}

    void Write(PacketBuffer& out) const override {}
};

int main(int argc, const char** argv)
{
#pragma region Server

    PacketServerConfiguration serverConfig;
    serverConfig.DisplayName = "Code Test Server";
    serverConfig.MaxPlayers = 1;

    PacketServer<PacketID> server(
        serverConfig,
        [](PacketServer<PacketID>::PacketInfo_t& info) -> PacketServer<PacketID>::Packet_ptr
        {
            switch(info.Header.ID)
            {
                case PacketID::MsgA:
                    return std::make_unique<MsgA>();
                case PacketID::MsgB:
                    return std::make_unique<MsgB>();
                default:
                    throw std::runtime_error("Unknown packet");
            }
        }
    );
    server.OnClientConnect = [](const PacketServer<PacketID>::Client_t& client) -> bool
    {
        std::cout << "Client connected" << std::endl;
        return true;
    };
    server.OnClientDisconnect = [](const PacketServer<PacketID>::Client_t& client) -> void
    {
        std::cout << "Client disconnected" << std::endl;
    };
    server.OnMessage = [](const PacketServer<PacketID>::Client_t& client, PacketServer<PacketID>::PacketInfo_t& info) -> void
    {
        std::cout << "Message received " << int(info.Header.ID) << std::endl;
    };
    server.Start();

#pragma endregion

#pragma region Client

    PacketClient<PacketID> client = PacketClient<PacketID>();
    client.Connect("localhost");

#pragma endregion

    client.Send(MsgA());
    client.Send(MsgB());
    client.Send(MsgA());

    std::cout << "Waiting for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

/*
PacketClient<PacketID> StartClient()
{
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
*/
