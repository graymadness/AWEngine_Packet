#pragma once

#include <istream>
#include <ostream>
#include <stdexcept>

#include "LocaleDoubleChar.hpp"
#include <AWEngine/Packet/PacketBuffer.hpp>

// CMakeLists.txt will enable this automatically when https://github.com/nlohmann/json is found as a target `nlohmann_json` (must be created before this library).
#ifdef AWE_PACKET_LIB_JSON
#   include <nlohmann/json.hpp>
#   include <sstream>
#endif

#ifndef AWE_LOCALE_INFO_SECOND_UPPER_CASE
#   define AWE_LOCALE_INFO_SECOND_UPPER_CASE 1
#endif

namespace AWEngine::Packet::Util
{
    /// ISO 639-1 standard language codes.
    /// Don't forget to call IsValid as this class won't throw any exception.
    /// Examples: en, en-US, en-GB, en-CA, cs-CZ
    struct LocaleInfo
    {
        LocaleDoubleChar LanguageCode;
        LocaleDoubleChar CountryCode;

    // Constructors
    public:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"
        inline LocaleInfo() noexcept : LanguageCode(), CountryCode() {}
        inline LocaleInfo(LocaleDoubleChar languageCode, LocaleDoubleChar countryCode = LocaleDoubleChar()) noexcept : LanguageCode(languageCode), CountryCode(countryCode) {}
        inline LocaleInfo(const std::string&) noexcept;
#pragma clang diagnostic pop

    // Utilities
    public:
        static const char Separator = '-';
        /// Checks whenever there is value or is empty.
        [[nodiscard]] inline operator bool() const noexcept { return LanguageCode; } // NOLINT(google-explicit-constructor)
        /// Checks whenever value is valid.
        /// Empty LanguageCode or CountryCode are valid but empty LanguageCode with non-empty CountryCode is not valid.
        [[nodiscard]] inline bool IsValid() const noexcept { return LanguageCode.IsValid() && CountryCode.IsValid() && !(!LanguageCode && CountryCode); }
        /// Current locale.
        /// May return empty.
        [[nodiscard]] static LocaleInfo Current() noexcept;
    };
    static_assert(sizeof(LocaleInfo) == 4);

    // std::stream
    inline std::istream& operator>>(std::istream&, LocaleInfo&) noexcept;
    /// May throw exception if provided with not-valid LocaleInfo
    inline std::ostream& operator<<(std::ostream&, LocaleInfo);

    // PacketBuffer
    inline PacketBuffer& operator>>(PacketBuffer&, LocaleInfo&);
    inline PacketBuffer& operator<<(PacketBuffer&, LocaleInfo);
}

#ifdef AWE_PACKET_LIB_JSON
namespace nlohmann
{
    template<>
    struct adl_serializer<AWEngine::Packet::Util::LocaleInfo>
    {
        static void to_json(json& j, const AWEngine::Packet::Util::LocaleInfo& value);
        static void from_json(const json& j, AWEngine::Packet::Util::LocaleInfo& value);
    };
}
#endif

namespace AWEngine::Packet::Util
{
    LocaleInfo::LocaleInfo(const std::string& str) noexcept
        : LanguageCode(),
          CountryCode()
    {
        if(str.length() >= 2)
        {
            LanguageCode = {str[0], str[1]};

            if(str.length() >= 5 && str[2] == Separator)
            {
                CountryCode = { str[3], str[4] };
            }
        }
    }

    std::istream& operator>>(std::istream& in, LocaleInfo& locale) noexcept
    {
        // Before separator
        in >> locale.LanguageCode;

        // After separator
        if(in.peek() == std::char_traits<char>::to_int_type(LocaleInfo::Separator))
        {
            in.ignore();
            in >> locale.CountryCode;
        }

        return in;
    }

    std::ostream& operator<<(std::ostream& out, LocaleInfo locale)
    {
        if(!locale.IsValid())
            throw std::runtime_error("Cannot write invalid locale");
        if(!locale)
            return out; // Empty won't write anything

        // Before separator
        locale.LanguageCode.Write(out, false);

        // After separator
        if(locale.CountryCode)
        {
            out << '-';
#if AWE_LOCALE_INFO_SECOND_UPPER_CASE == 0
            bool upperCase = false;
#else
            bool upperCase = true;
#endif
            locale.CountryCode.Write(out, upperCase);
        }

        return out;
    }

    /// Always reads 4 bytes
    inline PacketBuffer& operator>>(PacketBuffer& buffer, LocaleInfo& locale)
    {
        // Language Code
        locale.LanguageCode.LeftChar  = static_cast<char>(buffer.Read());
        locale.LanguageCode.RightChar = static_cast<char>(buffer.Read());
        if(locale.LanguageCode && locale.LanguageCode.IsValid())
            locale.LanguageCode = locale.LanguageCode.ChangeCase(false);
        else
        {
            locale = {};
            buffer.Skip(2); // Country Code
            return buffer;
        }

        // Country Code
        locale.CountryCode.LeftChar  = static_cast<char>(buffer.Read());
        locale.CountryCode.RightChar = static_cast<char>(buffer.Read());
        if(locale.CountryCode && locale.CountryCode.IsValid())
            locale.CountryCode = locale.CountryCode.ChangeCase(true);
        else
            locale.CountryCode = {};

        return buffer;
    }

    /// Always writes 4 bytes
    inline PacketBuffer& operator<<(PacketBuffer& buffer, LocaleInfo locale)
    {
        LocaleDoubleChar languageCode = locale.LanguageCode.ChangeCase(false);
        if(languageCode && languageCode.IsValid())
        {
            // Language Code
            buffer.Write(languageCode.LeftChar);
            buffer.Write(languageCode.RightChar);

            // Country Code
            LocaleDoubleChar countryCode = locale.CountryCode.ChangeCase(true);
            if(countryCode && countryCode.IsValid())
            {
                buffer.Write(countryCode.LeftChar);
                buffer.Write(countryCode.RightChar);
            }
            else
            {
                buffer.Write(0);
                buffer.Write(0);
            }
        }
        else
        {
            // Language Code
            buffer.Write(0);
            buffer.Write(0);
            // Country Code
            buffer.Write(0);
            buffer.Write(0);
        }

        return buffer;
    }
}

#ifdef AWE_PACKET_LIB_JSON
namespace nlohmann
{
    void adl_serializer<AWEngine::Packet::Util::LocaleInfo>::to_json(json& j, const AWEngine::Packet::Util::LocaleInfo& value)
    {
        using namespace AWEngine::Packet::Util;

        std::stringstream ss;
        ss << value;
        j = ss.str();
    }
    void adl_serializer<AWEngine::Packet::Util::LocaleInfo>::from_json(const json& j, AWEngine::Packet::Util::LocaleInfo& value)
    {
        using namespace AWEngine::Packet::Util;

        if(j.is_string())
            value = AWEngine::Packet::Util::LocaleInfo(j.get<std::string>());
        else
            value = AWEngine::Packet::Util::LocaleInfo(); // Empty
    }
}
#endif
