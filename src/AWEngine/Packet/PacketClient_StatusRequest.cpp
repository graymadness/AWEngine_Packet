#include "PacketClient.hpp"

#include "PacketWrapper.hpp"

#include "AWEngine/Packet/ToServer/Login/Init.hpp"
#include "AWEngine/Packet/ToClient/Login/ServerInfo.hpp"

namespace AWEngine::Packet
{
    ::AWEngine::Packet::ToClient::Login::ServerInfo PacketClient::GetServerStatus(const std::string& host, uint16_t port)
    {
        using tcp = asio::ip::tcp;

        /// Own context to not conflict with anything else in the game
        asio::io_context io_context;

        // New connection
        tcp::socket socket(io_context);
        tcp::resolver resolver(io_context);
        asio::connect(socket, resolver.resolve(host, std::to_string(port)));

        /// Temporary buffer to hopefully decrease amount of allocation during packet read/write
        Packet::PacketBuffer tmpBuffer;

        // Send info request
        Packet::PacketWrapper::WritePacket(socket, ::AWEngine::Packet::ToServer::Login::Init(::AWEngine::Packet::ToServer::Login::Init::NextStep::ServerInfo), tmpBuffer);

        // Read response
        {
            Packet::PacketID_t packetID;
            Packet::PacketWrapper::ReadPacket(socket, packetID, tmpBuffer);
            switch(packetID)
            {
                default:
                    throw std::runtime_error("Received unexpected packet from the server");

                case ::AWEngine::Packet::ToClient::Login::ServerInfo::s_PacketID():
                {
                    //return std::make_unique<::AWEngine::Packet::ToClient::Login::ServerInfo>(buff);
                    return ::AWEngine::Packet::ToClient::Login::ServerInfo(tmpBuffer);
                }
            }
        }
    }
}
