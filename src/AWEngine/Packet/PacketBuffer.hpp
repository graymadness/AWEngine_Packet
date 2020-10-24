#pragma once

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <climits>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>

#if CHAR_BIT != 8
    #error "unsupported char size"
#endif

#include "../../portable_endian.h"

namespace AWEngine::Packet
{
    class PacketBuffer
    {
    public:
        static const std::size_t StreamEofBufferStep = 1024;
        /// 4,294,967,295
        static const constexpr std::size_t MaxSize = (std::numeric_limits<uint32_t>::max)();

    public:
        /// Empty buffer
        PacketBuffer() = default;
        /// Read `in` until EOF is reached
        explicit PacketBuffer(std::istream& in)
        {
            std::size_t size = 0;
            std::size_t size_old;
            while(true)
            {
                // Read data by `StreamEofBufferStep` segments
                size_old = size;
                size += StreamEofBufferStep;
                m_Data.resize(size);
                in.read(reinterpret_cast<char*>(m_Data.data() + size_old), StreamEofBufferStep);

                if(in.eof())
                {
                    m_Data.resize(size_old + in.gcount()); // Discard memory to which was not written
                    break;
                }
            }
        }
        /// Read `in` for specified number of chars
        PacketBuffer(std::istream& in, uint32_t byteLength, bool exceptionOnLessChars = true)
        {
            m_Data.resize(byteLength);
            in.read(reinterpret_cast<char*>(m_Data.data()), byteLength);

            m_Data.resize(in.gcount());

            if(in.eof() && exceptionOnLessChars)
                throw std::runtime_error("Stream ended too soon");
        }
        /// From byte array
        PacketBuffer(const uint8_t* array, std::size_t arraySize)
        {
            if(arraySize > MaxSize)
                throw std::runtime_error("Source byte array is too big");

            m_Data.resize(arraySize);
            std::copy(array, array + arraySize, m_Data.data());
        }
        /// From byte array
        template<std::size_t N>
        PacketBuffer(const std::array<uint8_t, N>& data) : PacketBuffer(data.data(), data.size()) { }
        /// From byte vector
        PacketBuffer(const std::vector<uint8_t>& data) : PacketBuffer(data.data(), data.size()) { }
        /// From byte vector
        PacketBuffer(const std::vector<char>& data) : PacketBuffer(reinterpret_cast<const uint8_t*>(data.data()), data.size()) { }

    public:
        ~PacketBuffer() = default;

    private:
        /// Byte values
        std::vector<uint8_t> m_Data = {};
        /// Offset of first value in m_Data
        /// Used when reading from start of m_Data to not erase those values all the time which would cause re-allocation
        uint32_t m_StartOffset = 0;
    public:
        [[nodiscard]] inline const uint8_t* data() const noexcept { return m_Data.data() + m_StartOffset; }
        [[nodiscard]] inline uint32_t size() const noexcept { return m_Data.size() - m_StartOffset; }

    public:
        inline friend std::ostream& operator<<(std::ostream& out, PacketBuffer& buffer)
        {
            out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

            return out;
        }

    private:
        inline void RemoveStartOffset()
        {
            if(m_StartOffset != 0)
            {
                m_Data.erase(m_Data.begin(), m_Data.begin() + m_StartOffset);
                m_StartOffset = 0;
            }
        }

    // Write
    public:
        /// Write single byte
        inline void Write(uint8_t val)
        {
            RemoveStartOffset();

            if(m_Data.size() >= std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("Buffer is full");

            m_Data.emplace_back(val);
        }
        /// Write multiple bytes
        /// For size-prefixed array, use << operator
        inline void Write(uint32_t size, const uint8_t* val)
        {
            RemoveStartOffset();

            if(m_Data.size() + size > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("Buffer would be full");

            for(std::size_t i = 0; i < size; i++)
                m_Data.emplace_back(val[i]);
        }

    // Read
    public:
        /// Read single byte
        /// May throw exception
        [[nodiscard]] inline const uint8_t& Read()
        {
            if(m_StartOffset >= m_Data.size())
                throw std::runtime_error("Attempt to read outside of the buffer");

            return m_Data[m_StartOffset++]; // Read, then increment offset
        }
        /// Read single byte
        /// returns false if attempting to read outside of the buffer
        [[nodiscard]] inline bool Read(uint8_t& val)
        {
            if(m_StartOffset >= m_Data.size())
                return false;

            val = m_Data[m_StartOffset++]; // Read, then increment offset
            return true;
        }
        /// Read data to raw array
        [[nodiscard]] inline uint32_t ReadArray(uint8_t* data, uint32_t dataLength)
        {
            for(std::size_t i = 0; i < dataLength; i++)
                if(!Read(data[i]))
                    return i; // Filled only part of `data`

            return dataLength; // Filled whole `data`
        }

    // Peek
    public:
        /// Read single byte but not remove them
        [[nodiscard]] inline const uint8_t& Peek(uint8_t offset = 0) const
        {
            if(m_StartOffset + offset >= m_Data.size())
                throw std::runtime_error("Attempt to peek outside of the buffer");

            return m_Data[m_StartOffset + offset];
        }
        /// Peek data to raw array (read array but not remove them)
        [[nodiscard]] inline uint32_t PeekArray(uint8_t* data, uint32_t dataLength) const
        {
            if(static_cast<uint64_t>(dataLength) + m_StartOffset > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("Too long read. Requested data length + (internal) data offset > uint32::max");
            for(std::size_t i = 0, offset = m_StartOffset; i < dataLength; i++, offset++)
            {
                if(m_StartOffset + i >= m_Data.size())
                    return i; // Filled only part of `data`
                data[i] = m_Data[offset];
            }

            return dataLength; // Filled whole `data`
        }
        /// Peek data to raw array (read array but not remove them)
        [[nodiscard]] inline uint32_t PeekArray(uint8_t* data, uint32_t dataLength)
        {
            if(static_cast<uint64_t>(dataLength) + m_StartOffset > std::numeric_limits<uint32_t>::max())
                RemoveStartOffset();
            for(std::size_t i = 0, offset = m_StartOffset; i < dataLength; i++, offset++)
            {
                if(m_StartOffset + i >= m_Data.size())
                    return i; // Filled only part of `data`
                data[i] = m_Data[offset];
            }

            return dataLength; // Filled whole `data`
        }

    // Skip
    public:
        inline void Skip(uint8_t byteCount = 1) noexcept
        {
            m_StartOffset += byteCount;
        }

    // 8-bit number
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, int8_t value)
        {
            static_assert(sizeof(value) == 1);

            buffer.Write(value);

            return buffer;
        }
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, uint8_t value)
        {
            static_assert(sizeof(value) == 1);

            buffer.Write(value);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, int8_t& value)
        {
            static_assert(sizeof(value) == 1);

            value = buffer.Read();

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, uint8_t& value)
        {
            static_assert(sizeof(value) == 1);

            value = buffer.Read();

            return buffer;
        }

    // 16-bit number
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, int16_t value)
        {
            static_assert(sizeof(value) == 2);

            value = htole16(value);
            for(std::size_t i = 0; i < sizeof(value); i++)
                buffer.Write(reinterpret_cast<const uint8_t*>(value)[i]);

            return buffer;
        }
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, uint16_t value)
        {
            static_assert(sizeof(value) == 2);

            uint16_t v = htole16(value);
            for(std::size_t i = 0; i < sizeof(value); i++)
                buffer.Write(reinterpret_cast<const uint8_t*>(v)[i]);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, int16_t& value)
        {
            static_assert(sizeof(value) == 2);

            uint16_t v = 0;
            for(std::size_t i = 0; i < sizeof(value); i++)
                reinterpret_cast<char*>(&v)[i] = buffer.Read();
            value = le16toh(v);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, uint16_t& value)
        {
            static_assert(sizeof(value) == 2);

            for(std::size_t i = 0; i < sizeof(value); i++)
                reinterpret_cast<char*>(&value)[i] = buffer.Read();
            value = le16toh(value);

            return buffer;
        }

    // 32-bit number
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, int32_t value)
        {
            static_assert(sizeof(value) == 4);

            value = htole32(value);
            for(std::size_t i = 0; i < sizeof(value); i++)
                buffer.Write(reinterpret_cast<const uint8_t*>(value)[i]);

            return buffer;
        }
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, uint32_t value)
        {
            static_assert(sizeof(value) == 4);

            value = htole32(value);
            for(std::size_t i = 0; i < sizeof(value); i++)
                buffer.Write(reinterpret_cast<const uint8_t*>(value)[i]);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, int32_t& value)
        {
            static_assert(sizeof(value) == 4);

            uint32_t v = 0;
            for(std::size_t i = 0; i < sizeof(value); i++)
                reinterpret_cast<char*>(&v)[i] = buffer.Read();
            value = le32toh(v);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, uint32_t& value)
        {
            static_assert(sizeof(value) == 4);

            for(std::size_t i = 0; i < sizeof(value); i++)
                reinterpret_cast<char*>(&value)[i] = buffer.Read();
            value = le32toh(value);

            return buffer;
        }

    // 64-bit number
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, int64_t value)
        {
            static_assert(sizeof(value) == 8);

            value = htole64(value);
            for(std::size_t i = 0; i < sizeof(value); i++)
                buffer.Write(reinterpret_cast<const uint8_t*>(value)[i]);

            return buffer;
        }
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, uint64_t value)
        {
            static_assert(sizeof(value) == 8);

            value = htole64(value);
            for(std::size_t i = 0; i < sizeof(value); i++)
                buffer.Write(reinterpret_cast<const uint8_t*>(value)[i]);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, int64_t& value)
        {
            static_assert(sizeof(value) == 8);

            uint64_t v = 0;
            for(std::size_t i = 0; i < sizeof(value); i++)
                reinterpret_cast<char*>(&v)[i] = buffer.Read();
            value = le32toh(v);

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, uint64_t& value)
        {
            static_assert(sizeof(value) == 8);

            for(std::size_t i = 0; i < sizeof(value); i++)
                reinterpret_cast<char*>(&value)[i] = buffer.Read();
            value = le32toh(value);

            return buffer;
        }

    // float (32-bit floating point number)
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, float value)
        {
            static_assert(sizeof(value) == 4);

            return buffer << *reinterpret_cast<const uint32_t*>(&value);
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, float& value)
        {
            static_assert(sizeof(value) == 4);

            uint32_t v = 0;
            buffer >> v;
            value = *reinterpret_cast<const float*>(v);

            return buffer;
        }

    // double (64-bit floating point number)
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, double value)
        {
            static_assert(sizeof(value) == 8);

            return buffer << *reinterpret_cast<const uint32_t*>(&value);
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, double& value)
        {
            static_assert(sizeof(value) == 8);

            uint64_t v = 0;
            buffer >> v;
            value = *reinterpret_cast<const double*>(v);

            return buffer;
        }

    // string
    public:
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, const char* value)
        {
            static_assert(sizeof(char) == 1);

            if(value == nullptr)
            {
                buffer << static_cast<uint16_t>(0); // length

                return buffer;
            }

            std::size_t valueLength = strlen(value);
            if(valueLength > std::numeric_limits<uint16_t>::max()) // 65,535
                throw std::runtime_error("String is too long (exceeds uint16 limit)");

            // Content
            buffer << static_cast<uint16_t>(valueLength); // length

            for(std::size_t i = 0; i < valueLength; i++)
                buffer.Write(static_cast<uint8_t>(value[i]));

            return buffer;
        }
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, const std::string& value)
        {
            static_assert(sizeof(char) == 1);

            std::size_t valueLength = value.size();
            if(valueLength > std::numeric_limits<uint16_t>::max()) // 65,535
                throw std::runtime_error("String is too long (exceeds uint16 limit)");

            buffer << static_cast<uint16_t>(valueLength); // length

            // Content
            for(std::size_t i = 0; i < valueLength; i++)
                buffer.Write(static_cast<uint8_t>(value[i]));

            return buffer;
        }
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, std::string& value)
        {
            static_assert(sizeof(char) == 1);

            uint16_t length;
            buffer >> length;

#ifdef alloca
            if(length < 256)
            { // Stack
                char* arr = reinterpret_cast<char*>(alloca(length));
                if(length != buffer.ReadArray(reinterpret_cast<uint8_t*>(arr), length))
                    throw std::runtime_error("There were not data for whole string (length pointed outside of buffer)");

                value = std::string(arr, length);
            }
            else
#endif
            { // Heap
                char* arr = new char[length];
                if(length != buffer.ReadArray(reinterpret_cast<uint8_t*>(arr), length))
                    throw std::runtime_error("There were not data for whole string (length pointed outside of buffer)");

                value = std::string(arr, length);

                delete[] arr;
            }

            return buffer;
        }

    // array
    public:
        template<typename T>
        inline friend PacketBuffer& operator<<(PacketBuffer& buffer, const std::vector<T>& value)
        {
            std::size_t valueLength = value.size();
            if(valueLength > std::numeric_limits<uint16_t>::max()) // 65,535
                throw std::runtime_error("Vector is too long (exceeds uint16 limit)");

            buffer << static_cast<uint16_t>(valueLength); // length

            // Content
            for(std::size_t i = 0; i < valueLength; i++)
                buffer << value[i];

            return buffer;
        }
        template<typename T>
        inline friend PacketBuffer& operator>>(PacketBuffer& buffer, std::vector<T>& value)
        {
            uint16_t length;
            buffer >> length;

            std::size_t byteSize = length * sizeof(T);

            std::vector<T> arr(length);
            if(byteSize != buffer.ReadArray(reinterpret_cast<uint8_t*>(arr), byteSize))
                throw std::runtime_error("There were not data for whole array (length pointed outside of buffer)");

            value = arr;

            return buffer;
        }
    };
}

#pragma clang diagnostic pop
