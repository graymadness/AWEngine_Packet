#include "GameClient.hpp"

namespace AWEngine::Packet
{
    GameClient::GameClient()
    {
        //TODO Init networking
    }

    GameClient::~GameClient()
    {
        m_Closing = true;
        if(m_NetworkThread.joinable())
            m_NetworkThread.join();
    }

    bool GameClient::IsConnected() const noexcept
    {
        //TODO
    }

    void GameClient::Connect(const std::string& host, uint16_t port)
    {
        //TODO
        m_NetworkThread = std::thread([this]() -> void
                                      {
                                          while(!m_Closing)
                                          {
                                              //TODO
                                          }
                                      });
    }

    void GameClient::Disconnect()
    {
        if(!IsConnected())
            return;

        m_Closing = true;
        if(m_NetworkThread.joinable())
            m_NetworkThread.join();
    }
}
