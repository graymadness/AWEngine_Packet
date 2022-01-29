#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <memory>
#include <map>
#include <functional>

#include "ProtocolInfo.hpp"

namespace AWEngine::Packet
{
    struct PacketHeader
    {
        PacketID_t ID;
        uint16_t Size;
        PacketFlags Flags;
    };

    template<typename T, typename TPacketEnum>
    concept PacketConcept = std::is_base_of<IPacket<TPacketEnum>, T>::value;

    template<typename TPacketEnum>
    class IPacket
    {
    public:
        static_assert(std::is_enum<TPacketEnum>());
        static_assert(sizeof(TPacketEnum) == 1);

    public:
        explicit IPacket() = default;
        explicit IPacket(PacketBuffer& in) {}

        virtual ~IPacket() = default;

    public:
        virtual void Write(PacketBuffer& out) const = 0;
    };

    template<typename T, typename TPacketEnum>
    concept PacketConcept_ToClient = std::is_base_of<IPacket<TPacketEnum>, T>::value && T::s_Direction() == ::AWEngine::Packet::PacketDirection::ToClient;
    template<typename T, typename TPacketEnum>
    concept PacketConcept_ToServer = std::is_base_of<IPacket<TPacketEnum>, T>::value && T::s_Direction() == ::AWEngine::Packet::PacketDirection::ToServer;
}

#ifndef AWE_PACKET
#   define AWE_PACKET(a_packet_name, a_packet_enum)\
    class a_packet_name : public ::AWEngine::Packet::IPacket<a_packet_enum>
#endif

#ifndef AWE_PACKET_PARSER
#   define AWE_PACKET_PARSER(packet_name, a_packet_enum)\
    [](::AWEngine::Packet::PacketBuffer& in, ::AWEngine::Packet::PacketID_t id) -> std::unique_ptr<::AWEngine::Packet::IPacket<a_packet_enum>>\
    {\
        return std::make_unique<packet_name>(in);\
    }
#endif
