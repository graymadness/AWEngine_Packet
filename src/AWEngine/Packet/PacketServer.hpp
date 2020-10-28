#pragma once

#include <queue>

#include "TcpServer.hpp"
#include "../Util/ThreadSafeQueue.h"

namespace AWEngine::Packet
{
    typedef uint16_t SlotID_t;

    class IPacket;

    template<typename TPlayer>
    class PacketServer : public std::enable_shared_from_this<PacketServer<TPlayer>>
    {
    public:
        explicit PacketServer(const uint16_t port, const uint16_t maxPlayerCount)
            : m_TcpServer(AcceptCallback, port),
              m_Players(maxPlayerCount)
        {
            for(uint16_t slotIndex = 0; slotIndex < maxPlayerCount; slotIndex++)
                m_EmptyPlayerSlots.push(slotIndex);
        }

    public:
        ~PacketServer() = default;

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
            Player_t(std::weak_ptr<PacketServer<TPlayer>> server, SlotID_t slotID, asio::ip::tcp::socket socket)
                : m_Server(std::move(server)),
                  m_SlotID(slotID),
                  m_Socket(std::make_unique<asio::ip::tcp::socket>(std::move(socket))),
                  m_Player(std::make_shared<TPlayer>(slotID))
            {
                if(m_Server == nullptr)
                    throw std::runtime_error("Cannot create Player from NULL server");
            }

        private:
            std::weak_ptr<PacketServer<TPlayer>> m_Server;
            int32_t m_SlotID;
            std::unique_ptr<asio::ip::tcp::socket> m_Socket;
            Util::ThreadSafeQueue<std::vector<uint8_t>> m_OutQueue;
            std::shared_ptr<TPlayer> m_Player;
        public:
            [[nodiscard]] inline const std::weak_ptr<PacketServer<TPlayer>>& Server() const { return m_Server; }
            [[nodiscard]] inline int32_t SlotID() const { return m_SlotID; }
            [[nodiscard]] inline const std::unique_ptr<asio::ip::tcp::socket>& Socket() const { return m_Socket; }
            [[nodiscard]] inline const std::shared_ptr<TPlayer>& Player() const { return m_Player; }

        public:
            /// Checks whenever this player has valid Slot
            [[nodiscard]] inline bool HasSlot() const noexcept { return m_SlotID != -1; }

        public:
            void SendPacket(const std::shared_ptr<IPacket>& packet);
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
#ifdef DEBUG
                std::cerr << m_TcpServer.ServerPrefix() << ": No slot for a new player" << std::endl;
#endif
                return false;
            }

            m_PlayersMutex.lock();
            {
                SlotID_t slotID = m_EmptyPlayerSlots.front();
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

            m_PlayersMutex.lock();
            {
                m_Players[slotID] = nullptr;
                m_EmptyPlayerSlots.push(slotID);
            }
            m_PlayersMutex.unlock();
        }
    };
}
