/*
 * @file runtime/src/protocol/ServerJsonCodecReadNumber.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "protocol/ServerJsonCodecReadNumber.hpp"
#include "config/TextParse.hpp"

template <typename NumberType>
/*
 * @brief Documents the read number operation contract.
 * @param raw Input value used by this contract.
 * @param key Input value used by this contract.
 * @param out Input value used by this contract.
 * @return bool bltzr_protocol::ServerJsonCodec:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool bltzr_protocol::ServerJsonCodec::readNumber(std::string_view raw, std::string_view key,
                                                 NumberType& out)
{
    std::string token;
    if (!readToken(raw, key, token)) {
        return false;
    }
    return bltzr_text::parseNumber(token, out);
}

template bool bltzr_protocol::ServerJsonCodec::readNumber<int>(std::string_view raw,
                                                               std::string_view key, int& out);
template bool bltzr_protocol::ServerJsonCodec::readNumber<std::uint32_t>(std::string_view raw,
                                                                         std::string_view key,
                                                                         std::uint32_t& out);
template bool bltzr_protocol::ServerJsonCodec::readNumber<std::uint64_t>(std::string_view raw,
                                                                         std::string_view key,
                                                                         std::uint64_t& out);
template bool bltzr_protocol::ServerJsonCodec::readNumber<float>(std::string_view raw,
                                                                 std::string_view key, float& out);
template bool bltzr_protocol::ServerJsonCodec::readNumber<double>(std::string_view raw,
                                                                  std::string_view key,
                                                                  double& out);
