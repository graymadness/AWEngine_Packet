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
        std::cout << "Message received, Packet ID =" << int(info.Header.ID) << std::endl;

        if(client != nullptr)
            client->Send(MsgA());
    };
    server.Start();

#pragma endregion

#pragma region Client

    PacketClient<PacketID> client = PacketClient<PacketID>();
    client.Connect("localhost");
    std::this_thread::sleep_for(std::chrono::seconds(1));

#pragma endregion

    client.Send(MsgA());
    client.Send(MsgB());
    client.Send(MsgA());

    std::cout << "client.HasIncoming() = " << client.HasIncoming() << std::endl;

    std::cout << "Waiting for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "2 seconds passed" << std::endl;

    int processedMessages = server.Update(-1, false);
    std::cout << "Processed " << processedMessages << " messages" << std::endl;

    std::cout << "client.HasIncoming() = " << client.HasIncoming() << std::endl;
}
