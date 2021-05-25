#include "ProtocolInfo.hpp"

namespace AWEngine::Packet
{
#ifdef AWE_PACKET_PROTOCOL_VERSION
    const ProtocolVersion_t ProtocolInfo::ProtocolVersion = AWE_PACKET_PROTOCOL_VERSION;
#endif
#ifdef AWE_PACKET_GAME_NAME
    const GameName_t ProtocolInfo::GameName = ProtocolInfo::StringToArrayName<GameName_Length>(AWE_PACKET_GAME_NAME); // NOLINT(cert-err58-cpp)
#endif

    template<std::size_t N>
    std::array<char, N> ProtocolInfo::StringToArrayName(const std::string& strName)
    {
        static_assert(N > 0);
        if(strName.length() > N)
            throw std::runtime_error("Name is too long");

        std::array<char, N> rtn = {};
        std::copy(strName.data(), strName.data() + strName.length(), rtn.data());
        if(strName.length() != N)
            std::fill(rtn.data() + strName.length(), rtn.data() + N, '\0');
        return rtn;
    }
}
