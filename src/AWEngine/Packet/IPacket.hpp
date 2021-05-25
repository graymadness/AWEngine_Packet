#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <memory>
#include <map>
#include <functional>

#include "ProtocolInfo.hpp"
#include "PacketBuffer.hpp"

namespace AWEngine::Packet
{
    AWE_CLASS_UPTR(IPacket);

    typedef uint8_t PacketID_t;
    static const std::size_t PacketID_Count = static_cast<std::size_t>(std::numeric_limits<PacketID_t>::max()) + 1;

    typedef uint32_t ProtocolVersion_t;

    typedef std::function<IPacket_uptr(PacketBuffer&, PacketID_t)> PacketParser_t;

    AWE_STRUCT(PacketHeader)
    {
        PacketID_t ID;
        uint16_t Size;
        PacketFlags Flags;
    };

    AWE_CLASS_UPTR(IPacket)
    {
    public:
        explicit IPacket() = default;
        explicit IPacket(PacketBuffer& in) {}

        virtual ~IPacket() = default;

    public:
        virtual void Write(PacketBuffer& out) const = 0;

    public:
        /// Read packet header and rest of packet into `buffer`
        /// Returns whenever `header` was read successfully, states nothing about `buffer`
        /// Packet should be discarded if `buffer.empty()`
        static bool ReadPacket(std::istream& in, PacketHeader& header, PacketBuffer& buffer) noexcept;
        static void WritePacket(std::ostream& out, PacketID_t packetID, const PacketBuffer& buffer);

    public:
        template<::AWEngine::Packet::Direction DIR, PacketID_t ID>
        struct PacketInfo
        {
            [[nodiscard]] inline constexpr ::AWEngine::Packet::Direction Direction() const noexcept { return DIR; }
            [[nodiscard]] inline constexpr PacketID_t PacketID() const noexcept { return ID; }

            inline constexpr static ::AWEngine::Packet::Direction s_Direction() noexcept { return DIR; }
            inline constexpr static PacketID_t s_PacketID() noexcept { return ID; }
        };
    };

    template<typename T>
    concept PacketConcept = std::is_base_of<IPacket, T>::value;
}

#ifndef AWE_PACKET
#   define AWE_PACKET(packet_name, packet_direction, packet_id)\
    AWE_CLASS_PTR(packet_name) : public ::AWEngine::Packet::IPacket, ::AWEngine::Packet::IPacket::PacketInfo<::AWEngine::Packet::Direction::packet_direction, packet_id>
#endif

#ifndef AWE_PACKET_PARSER
#   define AWE_PACKET_PARSER(packet_name)\
    [](::AWEngine::Packet::PacketBuffer& in, ::AWEngine::Packet::PacketID_t id) -> ::AWEngine::Packet::IPacket_uptr\
    {\
        return std::weak_ptr<packet_name>(in);\
    }
#endif
