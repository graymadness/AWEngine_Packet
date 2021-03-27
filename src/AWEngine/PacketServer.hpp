#pragma once

#include <queue>

#include "TcpServer.hpp"
#include <AWEngine/Packet/IPacket.hpp>
#include <AWEngine/Util/ThreadSafeQueue.h>

namespace AWEngine
{
    typedef uint16_t SlotID_t;

    /// Extension of TCP Server to work over packets
    template<typename TPlayer>
    class PacketServer : public std::enable_shared_from_this<PacketServer<TPlayer>>
    {
    public:
        typedef std::map<uint32_t, std::array<Packet::PacketParser_t, 256>> VersionedPacketDefineMap_t;
    public:
        explicit PacketServer(
                const uint16_t port,
                const uint16_t maxPlayerCount,
                VersionedPacketDefineMap_t toClient,
                VersionedPacketDefineMap_t toServer
        )
            : m_TcpServer(AcceptCallback, port),
              m_Players(maxPlayerCount),
              m_ToClient(std::move(toClient)),
              m_ToServer(std::move(toServer))
        {
            for(uint16_t slotIndex = 0; slotIndex < maxPlayerCount; slotIndex++)
                m_EmptyPlayerSlots.push(slotIndex);
        }

    public:
        ~PacketServer() = default;

    private:
        VersionedPacketDefineMap_t m_ToClient;
        VersionedPacketDefineMap_t m_ToServer;

    public:
        struct Player_t : public std::enable_shared_from_this<Player_t>
        {
        public:
            Player_t()
                : m_Server(nullptr),
                  m_SlotID(-1),
                  m_Socket(),
                  m_Player(nullptr)
            {
            }
            Player_t(std::shared_ptr<PacketServer<TPlayer>> server, SlotID_t slotID, asio::ip::tcp::socket socket)
                : m_Server(std::move(server)),
                  m_SlotID(slotID),
                  m_Socket(std::make_unique<asio::ip::tcp::socket>(std::move(socket))),
                  m_Player(std::make_shared<TPlayer>(slotID))
            {
                if(m_Server == nullptr)
                    throw std::runtime_error("Cannot create Player from NULL server");
            }

        private:
            std::shared_ptr<PacketServer<TPlayer>> m_Server;
            int32_t m_SlotID;
            std::unique_ptr<asio::ip::tcp::socket> m_Socket;
            Util::ThreadSafeQueue<std::vector<uint8_t>> m_OutQueue;
            std::shared_ptr<TPlayer> m_Player;
        public:
            [[nodiscard]] inline const std::shared_ptr<PacketServer<TPlayer>>& Server() const { return m_Server; }
            [[nodiscard]] inline       int32_t                                 SlotID() const { return m_SlotID; }
            [[nodiscard]] inline const std::unique_ptr<asio::ip::tcp::socket>& Socket() const { return m_Socket; }
            [[nodiscard]] inline const std::shared_ptr<TPlayer>&               Player() const { return m_Player; }

        public:
            /// Checks whenever this player has valid Slot
            [[nodiscard]] inline bool HasSlot() const noexcept { return m_SlotID != -1; }

        private:
            void SendPacket(const Packet::IPacket_uptr& packet, Packet::PacketID_t packetId);
        public:
            void SendPacket(const Packet::IPacket_uptr& packet);
        private:
            void ProcessQueue();
        };

    private:
        TcpServer m_TcpServer;
    public:
        [[nodiscard]] inline const TcpServer& Tcp() noexcept { return m_TcpServer; }

    private:
        std::vector<std::shared_ptr<TPlayer>> m_Players;
        std::queue<uint16_t> m_EmptyPlayerSlots;
        std::mutex m_PlayersMutex;
    public:
        [[nodiscard]] inline const Player_t& GetSlot(SlotID_t slotID) const { return m_Players[slotID]; }
        [[nodiscard]] inline uint16_t GetPlayerCount() const noexcept { return m_Players.size() - m_EmptyPlayerSlots.size(); }

    private:
        bool AcceptCallback(asio::ip::tcp::socket socket)
        {
            if(m_EmptyPlayerSlots.empty())
            {
                AWE_DEBUG_CERR(m_TcpServer.ServerPrefix() << ": No slot for a new player");
                return false;
            }

            m_PlayersMutex.lock();
            {
                SlotID_t slotID = m_EmptyPlayerSlots.front();
                AWE_DEBUG_COUT(m_TcpServer.ServerPrefix() << ": New user in slot " << slotID);
                m_EmptyPlayerSlots.pop();

                m_Players[slotID] = std::make_shared<TPlayer>(
                        slotID,
                        std::make_unique<asio::ip::tcp::socket>(std::move(socket))
                );
            }
            m_PlayersMutex.unlock();

            return true;
        }
        void DisconnectCallback(const std::shared_ptr<TPlayer>& player)
        {
            // player's socket is already closed!

            SlotID_t slotID = player->m_SlotID;
            player->m_SlotID = -1;

            AWE_DEBUG_COUT(m_TcpServer.ServerPrefix() << ": User disconnected from slot " << slotID);

            m_PlayersMutex.lock();
            {
                m_Players[slotID] = nullptr;
                m_EmptyPlayerSlots.push(slotID);
            }
            m_PlayersMutex.unlock();
        }
    };
}
