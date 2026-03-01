#pragma once

#include "frontend/FrontendModuleApi.hpp"
#include "frontend/FrontendModuleHandle.hpp"
#include "platform/DynamicLibrary.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace grav_module {

constexpr std::size_t kErrorBufferSize = 1024u;

struct FrontendModuleHandle::Impl {
    grav_platform::DynamicLibrary library{};
    const FrontendModuleExportsV1 *exports = nullptr;
    std::uintptr_t stateOpaque = 0u;
    std::string path;
};

inline std::string errorFromBuffer(const std::array<char, kErrorBufferSize> &buffer, std::string_view fallback)
{
    std::string error = buffer.data();
    if (error.empty()) {
        error.assign(fallback.begin(), fallback.end());
    }
    return error;
}

inline void *toRawState(std::uintptr_t opaque)
{
    return reinterpret_cast<void *>(opaque);
}

} // namespace grav_module
