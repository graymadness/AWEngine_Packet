#pragma once

#include <memory>
#include <map>
#include <functional>

#include "Direction.hpp"
#include "PacketBuffer.hpp"
#include "PacketFlags.hpp"

namespace AWEngine::Packet
{
    AWE_CLASS_UPTR(IPacket);

    typedef uint8_t PacketID_t;
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
    };
}

#ifndef AWE_PACKET
#   define AWE_PACKET(packet_name) AWE_CLASS_PTR(packet_name) : public ::AWEngine::Packet::IPacket
#endif

#ifndef AWE_PACKET_PARSER
#   define AWE_PACKET_PARSER(packet_name) [](PacketBuffer& in, PacketID_t id) -> IPacket_uptr { return std::make_shared<packet_name>(in); }
#endif
