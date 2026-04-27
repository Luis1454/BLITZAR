// File: runtime/src/client/ClientModuleHandleInternal.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
#define GRAVITY_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"
#include "client/ClientModuleHandle.hpp"
#include "platform/DynamicLibrary.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
namespace grav_module {
constexpr std::size_t kErrorBufferSize = 1024u;
/// Description: Defines the ClientModuleHandle data or behavior contract.
struct ClientModuleHandle::Impl {
    grav_platform::DynamicLibrary library{};
    const ClientModuleExportsV1* exports = nullptr;
    ClientModuleOpaqueState state{};
    std::string path;
};
std::string errorFromBuffer(const std::array<char, kErrorBufferSize>& buffer,
                            std::string_view fallback);
} // namespace grav_module
#endif // GRAVITY_RUNTIME_SRC_CLIENT_CLIENTMODULEHANDLEINTERNAL_HPP_
