/*
 * @file runtime/include/protocol/ServerJsonCodecReadNumber.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef GRAVITY_SIM_SERVERJSONCODECREADNUMBER_HPP
#define GRAVITY_SIM_SERVERJSONCODECREADNUMBER_HPP
#include "protocol/ServerJsonCodec.hpp"

namespace grav_protocol {
extern template bool ServerJsonCodec::readNumber<int>(std::string_view raw, std::string_view key,
                                                      int& out);
extern template bool ServerJsonCodec::readNumber<std::uint32_t>(std::string_view raw,
                                                                std::string_view key,
                                                                std::uint32_t& out);
extern template bool ServerJsonCodec::readNumber<std::uint64_t>(std::string_view raw,
                                                                std::string_view key,
                                                                std::uint64_t& out);
extern template bool ServerJsonCodec::readNumber<float>(std::string_view raw, std::string_view key,
                                                        float& out);
extern template bool ServerJsonCodec::readNumber<double>(std::string_view raw, std::string_view key,
                                                         double& out);
} // namespace grav_protocol
#endif // GRAVITY_SIM_SERVERJSONCODECREADNUMBER_HPP
