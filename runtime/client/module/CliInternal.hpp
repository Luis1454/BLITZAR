/*
 * @file runtime/client/module/CliInternal.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
#define BLITZAR_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
#include "client/module/CliApi.hpp"
#include "client/module/CliBoundary.hpp"
#include "client/module/CliHandle.hpp"
#include "PltDynamicLibrary.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace bltzr_module {
constexpr std::size_t kErrorBufferSize = 1024u;

struct Handle::Impl {
    bltzr_platform::DynamicLibrary library{};
    std::optional<ExportsV1> exports;
    OpaqueState state{};
    std::string path;
};

std::string errorFromBuffer(const std::array<char, kErrorBufferSize>& buffer,
                            std::string_view fallback);
} // namespace bltzr_module
#endif // BLITZAR_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
