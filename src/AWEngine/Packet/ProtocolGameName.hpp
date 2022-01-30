#pragma once

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"

#include <array>
#include <initializer_list>
#include <istream>
#include <ostream>

#include "AWEngine/Packet/PacketBuffer.hpp"

namespace AWEngine::Packet
{
    typedef uint32_t ProtocolGameVersion;

    class ProtocolGameName
    {
    public:
        static const std::size_t CharLength = 8;
    public:
        union
        {
            char     CharData[CharLength];
            uint64_t NumericData;
        };

    public:
        inline constexpr ProtocolGameName() noexcept : NumericData(0) {}
        inline constexpr ProtocolGameName(const char*);
        inline constexpr ProtocolGameName(const std::array<char, CharLength>& data) noexcept { std::copy(data.begin(), data.end(), CharData); }
        inline constexpr ProtocolGameName(const std::initializer_list<char>&);
        inline constexpr ProtocolGameName(uint64_t numericValue) noexcept : NumericData(numericValue) {}

        //TODO comparison operators
    };
    static_assert(sizeof(ProtocolGameName) == ProtocolGameName::CharLength);

    inline std::ostream& operator<<(std::ostream& out, ProtocolGameName gameName);
    inline std::istream& operator>>(std::istream& in,  ProtocolGameName gameName);

    inline PacketBuffer& operator<<(PacketBuffer& out, ProtocolGameName gameName);
    inline PacketBuffer& operator>>(PacketBuffer& in,  ProtocolGameName gameName);
}

namespace AWEngine::Packet
{
    constexpr ProtocolGameName::ProtocolGameName(const char* str)
        : NumericData(0)
    {
        for(std::size_t i = 0; i < CharLength; i++)
        {
            CharData[i] = str[i];
            if(!str[i])
                break;
        }
    }

    constexpr ProtocolGameName::ProtocolGameName(const std::initializer_list<char>& data)
        : NumericData(0)
    {
        auto dataIter = data.begin();
        for(std::size_t i = 0; i < CharLength && i < data.size(); i++, dataIter++)
            CharData[i] = *dataIter;
    }

    inline std::ostream& operator<<(std::ostream& out, ProtocolGameName gameName)
    {
        out.write(gameName.CharData, ProtocolGameName::CharLength);
        return out;
    }
    inline std::istream& operator>>(std::istream& in,  ProtocolGameName gameName)
    {
        in.read(gameName.CharData, ProtocolGameName::CharLength);
        //TODO Fill unread amount by zeros (like in `PacketBuffer >> ProtocolGameName`)
        return in;
    }

    inline PacketBuffer& operator<<(PacketBuffer& out, ProtocolGameName gameName)
    {
        out.Write(ProtocolGameName::CharLength, reinterpret_cast<uint8_t*>(gameName.CharData));
        return out;
    }
    inline PacketBuffer& operator>>(PacketBuffer& in,  ProtocolGameName gameName)
    {
        auto readBytes = in.ReadArray(reinterpret_cast<uint8_t*>(gameName.CharData), ProtocolGameName::CharLength);
        if(readBytes != ProtocolGameName::CharLength)
            std::fill(gameName.CharData + readBytes, gameName.CharData + ProtocolGameName::CharLength, '\0');
        return in;
    }
}

#pragma clang diagnostic pop
