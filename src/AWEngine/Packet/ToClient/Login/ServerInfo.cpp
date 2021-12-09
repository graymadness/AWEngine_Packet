#include "ServerInfo.hpp"

namespace AWEngine::Packet::ToClient::Login
{
    const std::string ServerInfo::Field_MOTD          = "motd";
    const std::string ServerInfo::Field_ServerName    = "name";
    const std::string ServerInfo::Field_Website       = "website";
    const std::string ServerInfo::Field_ServerLocale  = "locale";
    const std::string ServerInfo::Field_OnlinePlayers = "players.online";
    const std::string ServerInfo::Field_MaxPlayers    = "players.max";
    const std::string ServerInfo::Field_Whitelist     = "whitelist";

    template<typename T>
    inline static T SimpleJsonParse(const std::string& json, const std::string& key) noexcept = delete;

    /// Looks for this pattern inside JSON: "key": "value"
    /// Supports
    /// Returns empty string on invalid JSON or when not found
    template<>
    inline std::string SimpleJsonParse<std::string>(const std::string& json, const std::string& key) noexcept //OPTIMIZE?
    {
        // Key with " surrounding it
        std::string jsonKey = "\"" + key + "\"";
        if(jsonKey.length() + 1 + 2 >= json.length())
            return std::string(); // Not enough space for value (1 = ':', 2 = '"')

        // Key index
        auto index = json.find(jsonKey);
        if(index == json.npos)
            return std::string(); // Not found
        if(index + jsonKey.length() + 1 + 2 >= json.length())
            return std::string(); // Not enough space for value (1 = ':', 2 = '"')

        // Start of value
        auto valueStart = index + jsonKey.length() + 1;
        for(; valueStart < json.length(); valueStart++)
        {
            char c = json[valueStart];
            switch(c)
            {
                case ':':
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                    //THINK std::isspace
                    continue; // Valid characters between " of key and value
                case '\"':
                    break;
                default:
                    return std::string(); // Invalid character
            }
        }
        if(valueStart >= json.length())
            return std::string(); // End of JSON

        // End of value
        auto valueEnd = valueStart + 1;
        for(; valueEnd < json.length(); valueEnd++)
        {
            char c = json[valueEnd];
            switch(c)
            {
                default:
                    continue;
                case '\"':
                    if(json[valueEnd - 1] == '\\')
                        continue; // Escaped = \"
                    break;
            }
        }

        // Extract value from JSON string using 2 indexes
        assert(valueStart < valueEnd);
        if(valueEnd == valueStart + 1)
            return std::string(); // Empty value
        return json.substr(valueStart, valueEnd - valueStart);
    }
    /// Returns 0 on error or empty value
    template<>
    inline int SimpleJsonParse<int>(const std::string& json, const std::string& key) noexcept //OPTIMIZE?
    {
        std::string str = SimpleJsonParse<std::string>(json, key);
        if(str.empty())
            return 0;

        try
        {
            std::size_t processedChars = 0;
            int value = std::stoi(str, &processedChars, 10);
            if(processedChars != str.length())
                return 0; // Not all characters were processed
            return value;
        }
        catch(...)
        {
            return 0;
        }
    }
    /// Returns 0 on error or empty value
    template<>
    inline std::optional<int> SimpleJsonParse<std::optional<int>>(const std::string& json, const std::string& key) noexcept //OPTIMIZE?
    {
        std::string str = SimpleJsonParse<std::string>(json, key);
        if(str.empty())
            return {};

        try
        {
            std::size_t processedChars = 0;
            int value = std::stoi(str, &processedChars, 10);
            if(processedChars != str.length())
                return {}; // Not all characters were processed
            return value;
        }
        catch(...)
        {
            return {};
        }
    }
    /// Returns bool based on first character of value (will return `true` for "yogurt").
    /// No value is returned on error or unknown character.
    template<>
    inline std::optional<bool> SimpleJsonParse<std::optional<bool>>(const std::string& json, const std::string& key) noexcept //OPTIMIZE?
    {
        std::string value = SimpleJsonParse<std::string>(json, key);
        if(value.empty())
            return {};

        char c = value[0];
        switch(c)
        {
            // Yes
            case 'y':
            case 'Y':
            // True
            case 't':
            case 'T':
            // 1
            case '1':
                return true;

            // No
            case 'n':
            case 'N':
            // False
            case 'f':
            case 'F':
            // 0
            case '0':
                return false;

            // Any other value
            default:
                return {};
        }
    }

    std::string         ServerInfo::MOTD()          const noexcept { return SimpleJsonParse<std::string>(        JsonString, Field_MOTD         ); }
    std::string         ServerInfo::ServerName()    const noexcept { return SimpleJsonParse<std::string>(        JsonString, Field_ServerName   ); }
    std::string         ServerInfo::Website()       const noexcept { return SimpleJsonParse<std::string>(        JsonString, Field_Website      ); }
    Util::LocaleInfo    ServerInfo::ServerLocale()  const noexcept { return SimpleJsonParse<std::string>(        JsonString, Field_ServerLocale ); }
    std::optional<int>  ServerInfo::OnlinePlayers() const noexcept { return SimpleJsonParse<int>(                JsonString, Field_OnlinePlayers); }
    int                 ServerInfo::MaxPlayers()    const noexcept { return SimpleJsonParse<int>(                JsonString, Field_MaxPlayers   ); }
    std::optional<bool> ServerInfo::Whitelist()     const noexcept { return SimpleJsonParse<std::optional<bool>>(JsonString, Field_Whitelist    ); }
}
