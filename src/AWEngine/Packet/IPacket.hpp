#pragma once
#include <AWEngine/Util/Core_Packet.hpp>

#include <memory>
#include <map>
#include <functional>

#include <asio.hpp>

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
        /// Read packet header and rest of packet into `buffer`.
        /// Returns whenever `header` was read successfully, states nothing about `buffer` (for that is `everythingOk` parameter).
        static bool ReadPacket(std::istream& in, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept;
        /// Read packet header and rest of packet into `buffer`.
        /// Returns `true` only when both header and content of the packet were read successfully.
        inline static bool ReadPacket(std::istream& in, PacketID_t& packetID, PacketBuffer& buffer) noexcept
        {
            bool everythingOk;
            return ReadPacket(in, packetID, buffer, everythingOk) && everythingOk;
        }
        /// Read packet header and rest of packet into `buffer`.
        /// Returns whenever `header` was read successfully, states nothing about `buffer`.
        static bool ReadPacket(asio::ip::tcp::socket& socket, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept;
        /// Read packet header and rest of packet into `buffer`.
        /// Returns `true` only when both header and content of the packet were read successfully.
        inline static bool ReadPacket(asio::ip::tcp::socket& socket, PacketID_t& packetID, PacketBuffer& buffer) noexcept
        {
            bool everythingOk;
            return ReadPacket(socket, packetID, buffer, everythingOk) && everythingOk;
        }
    public:
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static void WritePacket(std::ostream& out, PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static void WritePacket(std::ostream& out, const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            IPacket::WritePacket(out, P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static void WritePacket(std::ostream& out, const P& packet) { WritePacket(out, packet, PacketBuffer()); }
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static void WritePacket(asio::ip::tcp::socket& socket, PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static void WritePacket(asio::ip::tcp::socket& socket, const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            IPacket::WritePacket(socket, P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static void WritePacket(asio::ip::tcp::socket& socket, const P& packet) { WritePacket(socket, packet, PacketBuffer()); }

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
        return std::weak_ptr<packet_name>(in);\
    }
#endif
