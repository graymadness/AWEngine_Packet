#include "GameClient.hpp"

#include <AWEngine/Packet/ToClient/Ping.hpp>
#include <AWEngine/Packet/ToClient/Kick.hpp>

#include <AWEngine/Packet/ToServer/Pong.hpp>
#include <AWEngine/Packet/ToServer/Disconnect.hpp>

namespace AWEngine::Packet
{
    GameClient::GameClient(std::size_t maxOutputQueueSize)
            : m_IoContext(),
              m_Socket(m_IoContext),
              MaxReceivedQueueSize(maxOutputQueueSize)
    {
    }

    GameClient::~GameClient()
    {
        m_Closing = true;
        if(m_ReceiveThread.joinable())
            m_ReceiveThread.join();
    }

    void GameClient::Connect(const std::string& host, uint16_t port)
    {
        using tcp = asio::ip::tcp;
        tcp::resolver resolver(m_IoContext);

        asio::connect(m_Socket, resolver.resolve(host, std::to_string(port)));
        if(ConnectCallback)
            ConnectCallback();

        m_Closing = true;

        m_ReceiveThread = std::thread([this]() -> void
                                      {
                                          PacketBuffer tmpBuffer;

                                          try
                                          {
                                              while(!m_Closing)
                                              {
                                                  PacketID_t packetID;
                                                  bool everythingOk;
                                                  if(IPacket::ReadPacket(m_Socket, packetID, tmpBuffer, everythingOk))
                                                  {
                                                      if(everythingOk)
                                                      {
                                                          switch(packetID)
                                                          {
                                                              /// Ping packet requires fast response = respond first, then let the client process it.
                                                              case ::AWEngine::Packet::ToClient::Ping::s_PacketID():
                                                              {
                                                                  auto pingPacket = ::AWEngine::Packet::ToClient::Ping(tmpBuffer);
                                                                  Send(::AWEngine::Packet::ToServer::Pong(pingPacket.Payload));

                                                                  IPacket_uptr packet = std::make_unique<::AWEngine::Packet::ToClient::Ping>(pingPacket);

                                                                  // Should we place it into the queue?
                                                                  if(!PacketReceivedCallback || PacketReceivedCallback(packet))
                                                                  {
                                                                      if(EnableReceivedQueue)
                                                                      {
                                                                          if(ReceivedQueue.size() == MaxReceivedQueueSize)
                                                                              throw std::runtime_error("Queue for received packets is full");
                                                                          ReceivedQueue.emplace(std::move(packet));
                                                                      }
                                                                  }

                                                                  continue;
                                                              }

                                                              /// Kick packet is processed differently then rest of the packets as it ends the communication.
                                                              case ::AWEngine::Packet::ToClient::Kick::s_PacketID():
                                                              {
                                                                  auto kickPacket = ::AWEngine::Packet::ToClient::Kick(tmpBuffer);

                                                                  IPacket_uptr packet = std::make_unique<::AWEngine::Packet::ToClient::Kick>(kickPacket);

                                                                  // Should we place it into the queue?
                                                                  if(!PacketReceivedCallback || PacketReceivedCallback(packet))
                                                                  {
                                                                      if(EnableReceivedQueue)
                                                                      {
                                                                          if(ReceivedQueue.size() == MaxReceivedQueueSize)
                                                                              throw std::runtime_error("Queue for received packets is full");
                                                                          ReceivedQueue.emplace(std::move(packet));
                                                                      }
                                                                  }

                                                                  // Invoke disconnect callback
                                                                  if(DisconnectCallback)
                                                                  {
                                                                      DisconnectInfo kickInfo{
                                                                              DisconnectReason::Kicked,
                                                                              kickPacket.Type == ToClient::Kick::MessageType::Translatable,
                                                                              kickPacket.Message
                                                                      };
                                                                      DisconnectCallback(kickInfo);
                                                                  }

                                                                  return; // End the thread = no more read
                                                              }

                                                              default:
                                                              {
                                                                  //TODO Parse function (tmpBuffer -> packet)
                                                                  //TODO Packet parsed callback
                                                                  //TODO Put packet into output queue
                                                                  continue;
                                                              }
                                                          }
                                                      }
                                                      else
                                                          throw std::runtime_error("Failed to read packet body");
                                                  }
                                                  else
                                                      throw std::runtime_error("Failed to read packet header");
                                              }
                                          }
                                          catch(...)
                                          {
                                              m_Closing = true;

                                              // Disconnect the socket
                                              if(m_Socket.is_open())
                                              {
                                                  asio::error_code error;

                                                  m_Socket.shutdown(asio::socket_base::shutdown_both, error);
                                                  if(error)
                                                      std::cerr << "Failed to shutdown Socket after exception; Error Code " << error.value() << ": " << error.message() << std::endl;

                                                  m_Socket.close(error);
                                                  if(error)
                                                      std::cerr << "Failed to close Socket after exception; Error Code " << error.value() << ": " << error.message() << std::endl;
                                              }

                                              // Let the exception continue
                                              throw;
                                          }

                                          m_Closing = true;

                                          // Disconnect the socket
                                          if(m_Socket.is_open())
                                          {
                                              asio::error_code error;

                                              m_Socket.shutdown(asio::socket_base::shutdown_both, error);
                                              if(error)
                                                  std::cerr << "Failed to shutdown Socket; Error Code " << error.value() << ": " << error.message() << std::endl;

                                              m_Socket.close(error);
                                              if(error)
                                                  std::cerr << "Failed to close Socket; Error Code " << error.value() << ": " << error.message() << std::endl;
                                          }
                                      });
    }

    void GameClient::Disconnect()
    {
        if(!IsConnected())
            return;

        // Tell server that the disconnect was "by decision" and not an error
        Send(::AWEngine::Packet::ToServer::Disconnect());

        // Tell receiving thread to quit and wait for it
        m_Closing = true;
        if(m_ReceiveThread.joinable())
            m_ReceiveThread.join();
    }
}
