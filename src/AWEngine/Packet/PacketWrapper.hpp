#pragma once
#include <AWEngine/Packet/Util/Core_Packet.hpp>

#include <AWEngine/Packet/asio.hpp>
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

#ifdef AWE_PACKET_COROUTINE
    public: // Read Packet
        /// Read packet header and rest of packet into `buffer`.
        /// Returns whenever `header` was read successfully, states nothing about `buffer`.
        static asio::awaitable<bool> ReadPacketAsync(asio::tcp_socket& socket, PacketID_t& packetID, PacketBuffer& buffer, bool& everythingOk) noexcept;
        /// Read packet header and rest of packet into `buffer`.
        /// Returns `true` only when both header and content of the packet were read successfully.
        inline static asio::awaitable<bool> ReadPacketAsync(asio::tcp_socket& socket, PacketID_t& packetID, PacketBuffer& buffer) noexcept
        {
            bool everythingOk;
            co_return co_await PacketWrapper::ReadPacketAsync(socket, packetID, buffer, everythingOk) && everythingOk;
        }
#endif



    public: // Write Packet
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static void WritePacket(std::ostream& out, PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static void WritePacket(std::ostream& out, const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            PacketWrapper::WritePacket(out, P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static void WritePacket(std::ostream& out, const P& packet) { PacketWrapper::WritePacket(out, packet, PacketBuffer()); }
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static void WritePacket(asio::ip::tcp::socket& socket, PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static void WritePacket(asio::ip::tcp::socket& socket, const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            PacketWrapper::WritePacket(socket, P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static void WritePacket(asio::ip::tcp::socket& socket, const P& packet) { PacketWrapper::WritePacket(socket, packet, PacketBuffer()); }

    public: // Write Packet (async)
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static void WritePacketAsync(asio::ip::tcp::socket& socket, PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static void WritePacketAsync(asio::ip::tcp::socket& socket, const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            PacketWrapper::WritePacketAsync(socket, P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static void WritePacketAsync(asio::ip::tcp::socket& socket, const P& packet) { PacketWrapper::WritePacketAsync(socket, packet, PacketBuffer()); }

#ifdef AWE_PACKET_COROUTINE
    public: // Write Packet (async)
        /// Send packet with all the necessary options.
        /// Automatically decides on Packet Flags (like compression)
        static asio::awaitable<void> WritePacketAsync(asio::tcp_socket& socket, PacketID_t packetID, const PacketBuffer& buffer);
        template<PacketConcept P>
        inline static asio::awaitable<void> WritePacketAsync(asio::tcp_socket& socket, const P& packet, PacketBuffer buff)
        {
            buff.Clear();
            packet.Write(buff);
            return PacketWrapper::WritePacketAsync(socket, P::s_PacketID(), buff);
        }
        template<PacketConcept P>
        inline static asio::awaitable<void> WritePacketAsync(asio::tcp_socket& socket, const P& packet) { return PacketWrapper::WritePacketAsync(socket, packet, PacketBuffer()); }
#endif

    };
}
