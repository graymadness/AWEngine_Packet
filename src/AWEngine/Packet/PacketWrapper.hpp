#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <asio.hpp>

#include "IPacket.hpp"
#include "ProtocolInfo.hpp"
#include "PacketBuffer.hpp"

namespace AWEngine::Packet
{
    class PacketWrapper
    {
    public:
        PacketWrapper() = delete;

    public: // Read Packet
        /// Read packet header and rest of packet into `buffer`.
        /// Returns whenever `header` was read successfully, states nothing about `buffer` (for that is `everythingOk` parameter).
        static bool ReadPacket(std::istream& in, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept;
        /// Read packet header and rest of packet into `buffer`.
        /// Returns `true` only when both header and content of the packet were read successfully.
        inline static bool ReadPacket(std::istream& in, PacketID_t& packetID, PacketBuffer& buffer) noexcept
        {
            bool everythingOk;
            return PacketWrapper::ReadPacket(in, packetID, buffer, everythingOk) && everythingOk;
        }
        /// Read packet header and rest of packet into `buffer`.
        /// Returns whenever `header` was read successfully, states nothing about `buffer`.
        static bool ReadPacket(asio::ip::tcp::socket& socket, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept;
        /// Read packet header and rest of packet into `buffer`.
        /// Returns `true` only when both header and content of the packet were read successfully.
        inline static bool ReadPacket(asio::ip::tcp::socket& socket, PacketID_t& packetID, PacketBuffer& buffer) noexcept
        {
            bool everythingOk;
            return PacketWrapper::ReadPacket(socket, packetID, buffer, everythingOk) && everythingOk;
        }

    public: // Read Packet (async)
        /// Read packet header and rest of packet into `buffer`.
        static void ReadPacketAsync(
                asio::ip::tcp::socket& socket,
                std::function<void(PacketID_t, PacketBuffer)> receivedCallback,
                std::function<void(std::string)> failedCallback
                ) noexcept;



    public: // Write Packet
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static std::vector<uint8_t> WritePacket(PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static std::vector<uint8_t> WritePacket(const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            return PacketWrapper::WritePacket(P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static std::vector<uint8_t> WritePacket(const P& packet) { return PacketWrapper::WritePacket(packet, PacketBuffer()); }

    };
}
