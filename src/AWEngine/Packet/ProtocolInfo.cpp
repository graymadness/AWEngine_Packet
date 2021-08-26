#include "ProtocolInfo.hpp"

// To Client
#include "ToClient/Login/ServerInfo.hpp" // 0x00

#include "ToClient/Ping.hpp" // 0xFE
#include "ToClient/Kick.hpp" // 0xFF

// To Server
#include "ToServer/Login/Init.hpp" // 0x00

#include "ToServer/Pong.hpp"       // 0xFE
#include "ToServer/Disconnect.hpp" // 0xFF


namespace AWEngine::Packet
{
#ifdef AWE_PACKET_PROTOCOL_VERSION
    const ProtocolVersion_t ProtocolInfo::ProtocolVersion = AWE_PACKET_PROTOCOL_VERSION;
#endif
#ifdef AWE_PACKET_GAME_NAME
    const GameName_t ProtocolInfo::GameName = ProtocolInfo::StringToArrayName<GameName_Length>(AWE_PACKET_GAME_NAME); // NOLINT(cert-err58-cpp)
#endif

    inline static PacketParserList_t InitParsersToClient();
    inline static PacketParserList_t InitParsersToServer();

    PacketParserList_t ProtocolInfo::ParsersToClient = InitParsersToClient(); // NOLINT(cert-err58-cpp)
    PacketParserList_t ProtocolInfo::ParsersToServer = InitParsersToServer(); // NOLINT(cert-err58-cpp)

#ifdef AWE_PACKET_REGISTER_INLINE
#   error "AWE_PACKET_REGISTER_INLINE already exist"
#endif
#define AWE_PACKET_REGISTER_INLINE(list, packetType) \
    list[packetType::s_PacketID()] = AWE_PACKET_PARSER(packetType)

    PacketParserList_t InitParsersToClient()
    {
        PacketParserList_t list{};
        {
            // 0
            AWE_PACKET_REGISTER_INLINE(list, ::AWEngine::Packet::ToClient::Login::ServerInfo); // 0x00

            // F
            AWE_PACKET_REGISTER_INLINE(list, ::AWEngine::Packet::ToClient::Ping); // 0xFE
            AWE_PACKET_REGISTER_INLINE(list, ::AWEngine::Packet::ToClient::Kick); // 0xFF
        }
        return list;
    }

    PacketParserList_t InitParsersToServer()
    {
        PacketParserList_t list{};
        {
            // 0
            AWE_PACKET_REGISTER_INLINE(list, ::AWEngine::Packet::ToServer::Login::Init); // 0x00

            // F
            AWE_PACKET_REGISTER_INLINE(list, ::AWEngine::Packet::ToServer::Pong); // 0xFE
            AWE_PACKET_REGISTER_INLINE(list, ::AWEngine::Packet::ToServer::Disconnect); // 0xFF
        }
        return list;
    }

#ifndef AWE_PACKET_REGISTER_INLINE
#   error "AWE_PACKET_REGISTER_INLINE define does not exist"
#endif
#undef AWE_PACKET_REGISTER_INLINE

    template<std::size_t N>
    std::array<char, N> ProtocolInfo::StringToArrayName(const std::string& strName)
    {
        static_assert(N > 0);
        if(strName.length() > N)
            throw std::runtime_error("Name is too long");

        std::array<char, N> rtn = {};
        std::copy(strName.data(), strName.data() + strName.length(), rtn.data());
        if(strName.length() != N)
            std::fill(rtn.data() + strName.length(), rtn.data() + N, '\0');
        return rtn;
    }
}
