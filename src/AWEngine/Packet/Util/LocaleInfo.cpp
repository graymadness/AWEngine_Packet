#include "LocaleInfo.hpp"

namespace AWEngine::Packet::Util
{
    LocaleInfo LocaleInfo::Current() noexcept
    {
        std::locale l = std::locale("");
        std::string lname = l.name();
        if(lname.empty() || lname == "C")
            return {}; // Empty or C
        if(lname.length() < 2)
            return {}; // Too short
        auto locale = LocaleInfo({ lname[0], lname[1] }, {}); // Language code only
        if(lname.length() >= 5)
        {
            switch(lname[2])
            {
                case '-':
                case '_':
                    locale.CountryCode = { lname[3], lname[4] };
                    break;
            }
        }

        return locale;
    }
}
