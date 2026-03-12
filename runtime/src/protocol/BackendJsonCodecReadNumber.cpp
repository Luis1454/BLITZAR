#include "protocol/BackendJsonCodecReadNumber.hpp"
#include "config/TextParse.hpp"

template <typename NumberType>
bool grav_protocol::BackendJsonCodec::readNumber(
    std::string_view raw,
    std::string_view key,
    NumberType &out)
{
    std::string token;
    if (!readToken(raw, key, token)) {
        return false;
    }
    return grav_text::parseNumber(token, out);
}

template bool grav_protocol::BackendJsonCodec::readNumber<int>(
    std::string_view raw,
    std::string_view key,
    int &out);
template bool grav_protocol::BackendJsonCodec::readNumber<std::uint32_t>(
    std::string_view raw,
    std::string_view key,
    std::uint32_t &out);
template bool grav_protocol::BackendJsonCodec::readNumber<std::uint64_t>(
    std::string_view raw,
    std::string_view key,
    std::uint64_t &out);
template bool grav_protocol::BackendJsonCodec::readNumber<float>(
    std::string_view raw,
    std::string_view key,
    float &out);
template bool grav_protocol::BackendJsonCodec::readNumber<double>(
    std::string_view raw,
    std::string_view key,
    double &out);
