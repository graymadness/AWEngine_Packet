#include "PacketClient.hpp"

#include <AWEngine/Packet/ToClient/Ping.hpp>
#include <AWEngine/Packet/ToClient/Kick.hpp>

#include <AWEngine/Packet/ToServer/Pong.hpp>
#include <AWEngine/Packet/ToServer/Disconnect.hpp>

namespace AWEngine
{
    PacketClient::PacketClient(std::size_t maxOutputQueueSize)
            : m_IoContext(),
              m_Socket(m_IoContext),
              MaxReceivedQueueSize(maxOutputQueueSize)
    {
    }

    PacketClient::~PacketClient()
    {
        m_Closing = true;
        if(m_ReceiveThread.joinable())
            m_ReceiveThread.join();
    }

    void PacketClient::Connect(const std::string& host, uint16_t port)
    {
        if(IsConnected())
            throw std::runtime_error("Already connected");

        ReceivedQueue.clear();

        using tcp = asio::ip::tcp;
        tcp::resolver resolver(m_IoContext);

        asio::connect(m_Socket, resolver.resolve(host, std::to_string(port)));
        if(ConnectCallback)
            ConnectCallback();

        m_Closing = false;

        m_ReceivedPacketCount = 0;
        m_SentPacketCount = 0;

        m_ReceiveThread = std::thread([this]() -> void
                                      {
                                          using namespace ::AWEngine::Packet;
                                          PacketBuffer tmpBuffer;

                                          try
                                          {
                                              while(!m_Closing)
                                              {
                                                  PacketID_t packetID;
                                                  bool everythingOk;
                                                  if(PacketWrapper::ReadPacket(m_Socket, packetID, tmpBuffer, everythingOk))
                                                  {
                                                      if(everythingOk)
                                                      {
                                                          m_ReceivedPacketCount++;

                                                          switch(packetID)
                                                          {
                                                              /// Ping packet requires fast response = respond first, then let the client process it.
                                                              case ToClient::Ping::s_PacketID():
                                                              {
                                                                  auto pingPacket = ToClient::Ping(tmpBuffer);
                                                                  Send(ToServer::Pong(pingPacket.Payload));

                                                                  IPacket_uptr packet = std::make_unique<ToClient::Ping>(pingPacket);

                                                                  // Should we place it into the queue?
                                                                  if(!PacketReceivedCallback || PacketReceivedCallback(packet))
                                                                  {
                                                                      if(EnableReceivedQueue)
                                                                      {
                                                                          if(ReceivedQueue.size() == MaxReceivedQueueSize)
                                                                              throw std::runtime_error("Queue for received packets is full");
                                                                          ReceivedQueue.push_back(std::move(packet));
                                                                      }
                                                                  }

                                                                  continue;
                                                              }

                                                              /// Kick packet is processed differently then rest of the packets as it ends the communication.
                                                              case ToClient::Kick::s_PacketID():
                                                              {
                                                                  auto kickPacket = ToClient::Kick(tmpBuffer);

                                                                  IPacket_uptr packet = std::make_unique<ToClient::Kick>(kickPacket);

                                                                  // Should we place it into the queue?
                                                                  if(!PacketReceivedCallback || PacketReceivedCallback(packet))
                                                                  {
                                                                      if(EnableReceivedQueue)
                                                                      {
                                                                          if(ReceivedQueue.size() == MaxReceivedQueueSize)
                                                                              throw std::runtime_error("Queue for received packets is full");
                                                                          ReceivedQueue.push_back(std::move(packet));
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
                                                                  PacketParser_t parser = ProtocolInfo::ParsersToClient[packetID]; //TODO Parse function (tmpBuffer -> packet)
                                                                  if(!parser)
                                                                      throw std::runtime_error("Unsupported packet with ID=" + std::to_string(packetID));
                                                                  IPacket_uptr packet = parser(tmpBuffer, packetID);

                                                                  // Should we place it into the queue?
                                                                  if(!PacketReceivedCallback || PacketReceivedCallback(packet))
                                                                  {
                                                                      if(EnableReceivedQueue)
                                                                      {
                                                                          if(ReceivedQueue.size() == MaxReceivedQueueSize)
                                                                              throw std::runtime_error("Queue for received packets is full");
                                                                          ReceivedQueue.push_back(std::move(packet));
                                                                      }
                                                                  }
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

    void PacketClient::Disconnect()
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
