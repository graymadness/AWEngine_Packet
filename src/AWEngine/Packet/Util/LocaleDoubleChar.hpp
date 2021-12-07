#pragma once

#include <istream>
#include <ostream>
#include <stdexcept>

namespace AWEngine::Packet::Util
{
    struct LocaleDoubleChar
    {
        // Values
        union
        {
            /// Lover-case a-z characters
            char Value[2];
            struct
            {
                char LeftChar;
                char RightChar;
            };
            uint16_t NumericValue;
        };

        // Constructor
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
        inline LocaleDoubleChar() noexcept : LeftChar('\0'), RightChar('\0') { }
        inline LocaleDoubleChar(char left, char right) noexcept : LeftChar(left), RightChar(right) { }
#pragma clang diagnostic pop

        // Utilities
        /// Checks whenever there is value or is empty
        [[nodiscard]] inline operator bool() const noexcept { return NumericValue != 0; } // NOLINT(google-explicit-constructor)
        [[nodiscard]] inline bool IsValid() const noexcept;
        [[nodiscard]] inline LocaleDoubleChar ChangeCase(bool upperCase) const noexcept;

        // std::stream
        inline void Write(std::ostream&, bool upperCase) const;
    };
    static_assert(sizeof(LocaleDoubleChar) == 2);

    inline std::istream& operator>>(std::istream& in, LocaleDoubleChar& doubleChar) noexcept;
}

namespace AWEngine::Packet::Util
{
    bool LocaleDoubleChar::IsValid() const noexcept
    {
        if(LeftChar == '\0' && RightChar == '\0')
        {
            return true; // Empty
        }
        else
        {
            // Left
            if(LeftChar >= 'A' && LeftChar <= 'Z') // 65 - 90
            {
            }
            else if(LeftChar >= 'a' && LeftChar <= 'z') // 97 - 122
            {
            }
            else
                return false; // Invalid left character

            // Right
            if(RightChar >= 'A' && RightChar <= 'Z') // 65 - 90
            {
            }
            else if(RightChar >= 'a' && RightChar <= 'z') // 97 - 122
            {
            }
            else
                return false; // Invalid right character

            // Both left and right characters are valid
            return true;
        }
    }

    LocaleDoubleChar LocaleDoubleChar::ChangeCase(bool upperCase) const noexcept
    {
        if(!IsValid())
            return {}; // Invalid
        if(!operator bool())
            return {}; // Empty

        static const char lowerToUpperDiff = 'a' - 'A';
        LocaleDoubleChar outDoubleChar = {};

        if(upperCase)
            outDoubleChar.LeftChar = static_cast<char>((LeftChar >= 'a' ? LeftChar - lowerToUpperDiff : LeftChar));
        else
            outDoubleChar.LeftChar = static_cast<char>((LeftChar <= 'Z' ? LeftChar + lowerToUpperDiff : LeftChar));

        if(upperCase)
            outDoubleChar.RightChar = static_cast<char>((RightChar >= 'a' ? RightChar - lowerToUpperDiff : RightChar));
        else
            outDoubleChar.RightChar = static_cast<char>((RightChar <= 'Z' ? RightChar + lowerToUpperDiff : RightChar));

        return outDoubleChar;
    }

    void LocaleDoubleChar::Write(std::ostream& out, bool upperCase) const
    {
        if(!IsValid())
            throw std::runtime_error("Cannot write invalid value");

        LocaleDoubleChar correctCase = ChangeCase(upperCase);

        if(correctCase)
            out << correctCase.LeftChar << correctCase.RightChar;
    }

    std::istream& operator>>(std::istream& in, LocaleDoubleChar& doubleChar) noexcept
    {
        char chars[2];
        in.read(chars, 2);
        doubleChar = LocaleDoubleChar(chars[0], chars[1]);
        return in;
    }
}
