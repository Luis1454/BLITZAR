#pragma once

#include "frontend/FrontendModuleApi.hpp"
#include "frontend/FrontendModuleBoundary.hpp"
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
    FrontendModuleOpaqueState state{};
    std::string path;
};

std::string errorFromBuffer(const std::array<char, kErrorBufferSize> &buffer, std::string_view fallback);

} // namespace grav_module
