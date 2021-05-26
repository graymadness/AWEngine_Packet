#include "PacketBuffer.hpp"

namespace AWEngine::Packet
{
    void PacketBuffer::Load(std::istream& in, uint32_t byteLength, bool exceptionOnLessChars)
    {
        if(byteLength == 0)
            return;
        if(!in.good())
            throw std::runtime_error("Input stream is not in a good shape");

        std::size_t size_old = m_Data.size();
        m_Data.resize(size_old + byteLength);
        in.read(reinterpret_cast<char*>(m_Data.data() + size_old), byteLength);
        std::size_t bytesTransferred = in.gcount();

        if(!in.good() && !in.eof())
            throw std::runtime_error("Input stream is not in a good shape after read");

        if(exceptionOnLessChars)
        {
            if(in.eof() || bytesTransferred != byteLength)
                throw std::runtime_error("Stream ended too soon");
        }
        else
        {
            if(bytesTransferred != byteLength)
                m_Data.resize(size_old + bytesTransferred);
        }
    }

    std::size_t PacketBuffer::LoadAll(std::istream& in)
    {
        if(!in.good())
            throw std::runtime_error("Input stream is not in a good shape");

        std::size_t size = 0, size_old;
        std::size_t totalTransferred = 0;
        while(true)
        {
            // Read data by `StreamEofBufferStep` segments
            size_old = size;
            size += StreamEofBufferStep;
            m_Data.resize(size);
            in.read(reinterpret_cast<char*>(m_Data.data() + size_old), StreamEofBufferStep);
            std::size_t bytesTransferred = in.gcount();
            totalTransferred += bytesTransferred;

            if(!in.good() && !in.eof())
                throw std::runtime_error("Input stream is not in a good shape after read");

            if(in.eof() || bytesTransferred != StreamEofBufferStep)
            {
                m_Data.resize(size_old + bytesTransferred); // Discard memory to which was not written
                break;
            }
        }
        return totalTransferred;
    }

    void PacketBuffer::Load(asio::ip::tcp::socket& socket, uint32_t byteLength, bool exceptionOnLessChars)
    {
        if(byteLength == 0)
            return;

        std::size_t size_old = m_Data.size();
        m_Data.resize(size_old + byteLength);
        std::size_t bytesTransferred = asio::read(socket, asio::buffer(reinterpret_cast<char*>(m_Data.data() + size_old), byteLength));

        if(exceptionOnLessChars)
        {
            if(bytesTransferred != byteLength)
                throw std::runtime_error("Data ended too soon");
        }
        else
        {
            if(bytesTransferred != byteLength)
                m_Data.resize(size_old + bytesTransferred); // Discard memory to which was not written
        }
    }
}
