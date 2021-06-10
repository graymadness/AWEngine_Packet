#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <memory>
#include <map>
#include <functional>

#include "ProtocolInfo.hpp"

namespace AWEngine::Packet
{
    AWE_STRUCT(PacketHeader)
    {
        PacketID_t ID;
        uint16_t Size;
        PacketFlags Flags;
    };

    template<typename T>
    concept PacketConcept = std::is_base_of<IPacket, T>::value;

    AWE_CLASS_UPTR(IPacket)
    {
    public:
        explicit IPacket() = default;
        explicit IPacket(PacketBuffer& in) {}

        virtual ~IPacket() = default;

    public:
        static std::size_t CompressionThreshold;

    public:
        virtual void Write(PacketBuffer& out) const = 0;

    public:
        /// Struct used to identify the packet
        /// Required to be able to send the packet as it defines the ID of the packet.
        template<::AWEngine::Packet::Direction DIR, PacketID_t ID>
        struct PacketInfo
        {
            [[nodiscard]] inline constexpr        ::AWEngine::Packet::Direction Direction() const noexcept { return DIR; }
            [[nodiscard]] inline constexpr        PacketID_t                    PacketID()  const noexcept { return ID; }

            [[nodiscard]] inline consteval static ::AWEngine::Packet::Direction s_Direction() noexcept { return DIR; }
            [[nodiscard]] inline consteval static PacketID_t                    s_PacketID()  noexcept { return ID; }
        };
    };

    template<typename T>
    concept PacketConcept_ToClient = std::is_base_of<IPacket, T>::value && T::s_Direction() == ::AWEngine::Packet::Direction::ToClient;
    template<typename T>
    concept PacketConcept_ToServer = std::is_base_of<IPacket, T>::value && T::s_Direction() == ::AWEngine::Packet::Direction::ToServer;
}

#ifndef AWE_PACKET
#   define AWE_PACKET(packet_name, packet_direction, packet_id)\
    AWE_CLASS_UPTR(packet_name) : public ::AWEngine::Packet::IPacket, public ::AWEngine::Packet::IPacket::PacketInfo<::AWEngine::Packet::Direction::packet_direction, packet_id>
#endif

#ifndef AWE_PACKET_PARSER
#   define AWE_PACKET_PARSER(packet_name)\
    [](::AWEngine::Packet::PacketBuffer& in, ::AWEngine::Packet::PacketID_t id) -> ::AWEngine::Packet::IPacket_uptr\
    {\
        return std::make_unique<packet_name>(in);\
    }
#endif
