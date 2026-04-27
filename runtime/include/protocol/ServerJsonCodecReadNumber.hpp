// File: runtime/include/protocol/ServerJsonCodecReadNumber.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_SIM_SERVERJSONCODECREADNUMBER_HPP
#define GRAVITY_SIM_SERVERJSONCODECREADNUMBER_HPP
#include "protocol/ServerJsonCodec.hpp"

namespace grav_protocol {
/// Description: Describes the read number<int> operation contract.
extern template bool ServerJsonCodec::readNumber<int>(std::string_view raw, std::string_view key,
                                                      int& out);
/// Description: Describes the uint32 t> operation contract.
extern template bool ServerJsonCodec::readNumber<std::uint32_t>(std::string_view raw,
                                                                std::string_view key,
                                                                std::uint32_t& out);
/// Description: Describes the uint64 t> operation contract.
extern template bool ServerJsonCodec::readNumber<std::uint64_t>(std::string_view raw,
                                                                std::string_view key,
                                                                std::uint64_t& out);
/// Description: Describes the read number<float> operation contract.
extern template bool ServerJsonCodec::readNumber<float>(std::string_view raw, std::string_view key,
                                                        float& out);
/// Description: Describes the read number<double> operation contract.
extern template bool ServerJsonCodec::readNumber<double>(std::string_view raw, std::string_view key,
                                                         double& out);
} // namespace grav_protocol
#endif // GRAVITY_SIM_SERVERJSONCODECREADNUMBER_HPP
