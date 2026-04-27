// File: runtime/src/protocol/ServerJsonCodecReadNumber.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "protocol/ServerJsonCodecReadNumber.hpp"
#include "config/TextParse.hpp"

template <typename NumberType>
/// Description: Describes the read number operation contract.
bool grav_protocol::ServerJsonCodec::readNumber(std::string_view raw, std::string_view key,
                                                NumberType& out)
{
    std::string token;
    if (!readToken(raw, key, token)) {
        return false;
    }
    return grav_text::parseNumber(token, out);
}

/// Description: Describes the read number<int> operation contract.
template bool grav_protocol::ServerJsonCodec::readNumber<int>(std::string_view raw,
                                                              std::string_view key, int& out);
/// Description: Describes the uint32 t> operation contract.
template bool grav_protocol::ServerJsonCodec::readNumber<std::uint32_t>(std::string_view raw,
                                                                        std::string_view key,
                                                                        std::uint32_t& out);
/// Description: Describes the uint64 t> operation contract.
template bool grav_protocol::ServerJsonCodec::readNumber<std::uint64_t>(std::string_view raw,
                                                                        std::string_view key,
                                                                        std::uint64_t& out);
/// Description: Describes the read number<float> operation contract.
template bool grav_protocol::ServerJsonCodec::readNumber<float>(std::string_view raw,
                                                                std::string_view key, float& out);
/// Description: Describes the read number<double> operation contract.
template bool grav_protocol::ServerJsonCodec::readNumber<double>(std::string_view raw,
                                                                 std::string_view key, double& out);
