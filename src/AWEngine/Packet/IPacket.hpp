#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <memory>
#include <map>
#include <functional>

#include "ProtocolInfo.hpp"

namespace AWEngine::Packet
{
    template<typename TPacketID>
    struct PacketHeader
    {
    public:
        static_assert(sizeof(TPacketID) == 1);
        static_assert(sizeof(PacketFlags) == 1);

    public:
        TPacketID ID;
        PacketFlags Flags;
        uint16_t    Size;
    };
    static_assert(sizeof(PacketHeader<uint8_t>) == 4);

    template<typename T, typename TPacketID>
    concept PacketConcept = std::is_base_of<IPacket<TPacketID>, T>::value;

    template<typename TPacketID>
    class IPacket
    {
    public:
        static_assert(std::is_enum<TPacketID>());
        static_assert(sizeof(TPacketID) == 1);

    public:
        explicit IPacket(TPacketID id) : ID(id) {}

        virtual ~IPacket() = default;

    public:
        const TPacketID ID;

    public:
        virtual void Write(PacketBuffer& out) const = 0;
    };

    template<typename T, typename TPacketID>
    concept PacketConcept_ToClient = std::is_base_of<IPacket<TPacketID>, T>::value && T::s_Direction() == ::AWEngine::Packet::PacketDirection::ToClient;
    template<typename T, typename TPacketID>
    concept PacketConcept_ToServer = std::is_base_of<IPacket<TPacketID>, T>::value && T::s_Direction() == ::AWEngine::Packet::PacketDirection::ToServer;
}

#ifndef AWE_PACKET_PARSER
#   define AWE_PACKET_PARSER(packet_name, a_packet_enum)\
    [](::AWEngine::Packet::PacketBuffer& in, ::AWEngine::Packet::PacketID_t id) -> std::unique_ptr<::AWEngine::Packet::IPacket<a_packet_enum>>\
    {\
        return std::make_unique<packet_name>(in);\
    }
#endif
