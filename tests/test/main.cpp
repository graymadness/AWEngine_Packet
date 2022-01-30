#include "AWEngine/Packet/PacketServer.hpp"
#include "AWEngine/Packet/PacketClient.hpp"

enum class PacketID : uint8_t
{
    MsgA = 0u,
    MsgB = 1u,
    MsgC = 2u,

    //----------------

    Ping       = 0xF1u,
    Pong       = Ping, // Can be same because of packet direction

    Kick       = 0xFFu,
    Disconnect = Kick // Can be same because of packet direction
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

struct MsgC : public IPacket<PacketID>
{
    MsgC() : IPacket<PacketID>(PacketID::MsgC) {}

    void Write(PacketBuffer& out) const override {}
};

int main()
{
#pragma region Server

    PacketServerConfiguration serverConfig;
    serverConfig.DisplayName = "Code Test Server";
    serverConfig.MaxPlayers = 1;

    typedef PacketServer<PacketID, PacketID::Ping, PacketID::Kick> PacketServer_t;
    PacketServer_t server(
        serverConfig,
        [](PacketServer_t::PacketInfo_t& info) -> PacketServer_t::Packet_ptr
        {
            switch(info.Header.ID)
            {
                case PacketID::MsgA:
                    return std::make_unique<MsgA>();
                case PacketID::MsgB:
                    return std::make_unique<MsgB>();
                case PacketID::MsgC:
                    return std::make_unique<MsgC>();
                case PacketID::Pong:
                    std::cout << "Received Pong" << std::endl;
                    return {}; // No need to respond, server does it automatically
                case PacketID::Disconnect:
                    return {};
                default:
                    throw std::runtime_error("Unknown packet with ID=" + std::to_string(int(info.Header.ID)));
            }
        }
    );
    server.OnClientConnect = [&](const PacketServer_t::Client_t& client) -> bool
    {
        std::cout << "Client " << client->RemoteEndpoint() << " connected" << std::endl;
        return true;
    };
    server.OnClientDisconnect = [&](const PacketServer_t::Client_t& client) -> void
    {
        std::cout << "Client " << client->RemoteEndpoint() << " disconnected" << std::endl;
    };
    server.OnMessage = [&](const PacketServer_t::Client_t& client, PacketServer_t::PacketInfo_t& info) -> void
    {
        std::cout << "Message received from " << client->RemoteEndpoint() << ", Packet ID = " << int(info.Header.ID) << std::endl;
    };
    server.OnPacket = [&](const PacketServer_t::Client_t& client, PacketServer_t::Packet_ptr& packet) -> void
    {
        std::cout << "Packet  received from " << client->RemoteEndpoint() << ", Packet ID = " << int(packet->ID) << std::endl;

        client->Send(packet);
    };
    server.Start();

#pragma endregion

    typedef PacketClient<PacketID, PacketID::Pong, PacketID::Disconnect> PacketClient_t;

#pragma region Client

    PacketClient_t client = PacketClient_t();
    client.Connect("localhost");
    client.WaitForConnect();

#pragma endregion

#pragma region Client2

    PacketClient_t client2 = PacketClient_t();
    client2.Connect("localhost");
    client2.WaitForConnect();

#pragma endregion

    // Send testing messages
    {
        client2.Send(MsgC());

        client.Send(MsgA());
        client.Send(MsgB());
        client.Send(MsgA());
        client.Send(MsgB());

        client2.Send(MsgC());
    }

    // Wait for messages to get from client to server
    std::cout << "Waiting for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "2 seconds passed" << std::endl;

    std::cout << std::endl;

    // Process messages on server
    std::size_t processedMessages = server.Update(-1, false);
    std::cout << "Processed " << processedMessages << " messages" << std::endl;

    std::cout << std::endl;

    std::cout << "Waiting for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "2 seconds passed" << std::endl;

    std::cout << std::endl;

    std::cout << "client.HasIncoming() = " << (client.HasIncoming() ? "yes" : "no") << " (" << client.Incoming().size() << ")" << std::endl;
    std::cout << "client2.HasIncoming() = " << (client2.HasIncoming() ? "yes" : "no") << " (" << client2.Incoming().size() << ")" << std::endl;

    std::cout << std::endl;

    std::cout << "client sent " << client.Connection().SentPacketCount() << " packets and received " << client.Connection().ReceivedPacketCount() << " packets" << std::endl;
    while(client.HasIncoming())
        std::cout << "client: " << int(client.Incoming().pop_front().second.Header.ID) << std::endl;

    std::cout << std::endl;

    std::cout << "client2 sent " << client2.Connection().SentPacketCount() << " packets and received " << client2.Connection().ReceivedPacketCount() << " packets" << std::endl;
    while(client2.HasIncoming())
        std::cout << "client2: " << int(client2.Incoming().pop_front().second.Header.ID) << std::endl;

    std::cout << std::endl;

    std::cout << "client2 Disconnect" << std::endl;
    client2.Disconnect();

    std::cout << std::endl;

    std::cout << "Waiting for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "2 seconds passed" << std::endl;

    std::cout << std::endl;

    // Process messages on server
    processedMessages = server.Update(-1, false);
    std::cout << "Processed " << processedMessages << " messages" << std::endl;

    std::cout << std::endl;

    std::cout << "Sending KeepAlive..." << std::endl;
    server.SendKeepAlive();

    std::cout << std::endl;

    std::cout << "Waiting for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "2 seconds passed" << std::endl;

    std::cout << std::endl;

    std::cout << "client.HasIncoming() = " << (client.HasIncoming() ? "yes" : "no") << " (" << client.Incoming().size() << ")" << std::endl;
    std::cout << "client2.HasIncoming() = " << (client2.HasIncoming() ? "yes" : "no") << " (" << client2.Incoming().size() << ")" << std::endl;

    // Process messages on server
    processedMessages = server.Update(-1, false);
    std::cout << "Processed " << processedMessages << " messages" << std::endl;

    std::cout << std::endl;
}
