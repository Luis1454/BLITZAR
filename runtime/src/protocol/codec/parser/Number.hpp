/*
 * @file runtime/src/protocol/codec/parser/Number.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_SRC_PROTOCOL_CODEC_PARSER_NUMBER_HPP
#define BLITZAR_RUNTIME_SRC_PROTOCOL_CODEC_PARSER_NUMBER_HPP
#include "protocol/codec/JsonCodec.hpp"

namespace bltzr_protocol {
extern template bool JsonCodec::readNumber<int>(std::string_view raw, std::string_view key,
                                                      int& out);
extern template bool JsonCodec::readNumber<std::uint32_t>(std::string_view raw,
                                                                std::string_view key,
                                                                std::uint32_t& out);
extern template bool JsonCodec::readNumber<std::uint64_t>(std::string_view raw,
                                                                std::string_view key,
                                                                std::uint64_t& out);
extern template bool JsonCodec::readNumber<float>(std::string_view raw, std::string_view key,
                                                        float& out);
extern template bool JsonCodec::readNumber<double>(std::string_view raw, std::string_view key,
                                                         double& out);
} // namespace bltzr_protocol
#endif // BLITZAR_RUNTIME_SRC_PROTOCOL_CODEC_PARSER_NUMBER_HPP
