/*
 * @file runtime/src/client/module/Internal.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
#define BLITZAR_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
#include "client/module/Api.hpp"
#include "client/module/Boundary.hpp"
#include "client/module/Handle.hpp"
#include "platform/DynamicLibrary.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace bltzr_module {
constexpr std::size_t kErrorBufferSize = 1024u;

struct Handle::Impl {
    bltzr_platform::DynamicLibrary library{};
    const ExportsV1* exports = nullptr;
    OpaqueState state{};
    std::string path;
};

std::string errorFromBuffer(const std::array<char, kErrorBufferSize>& buffer,
                            std::string_view fallback);
} // namespace bltzr_module
#endif // BLITZAR_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
