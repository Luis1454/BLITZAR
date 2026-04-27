// File: engine/include/platform/DynamicLibrary.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
#include "platform/PlatformErrors.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace grav_platform {
/// Description: Defines the DynamicLibrary data or behavior contract.
class DynamicLibrary {
public:
    /// Description: Describes the dynamic library operation contract.
    DynamicLibrary();
    /// Description: Releases resources owned by DynamicLibrary.
    ~DynamicLibrary();
    /// Description: Describes the dynamic library operation contract.
    DynamicLibrary(const DynamicLibrary&) = delete;
    /// Description: Describes the operator= operation contract.
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    /// Description: Describes the dynamic library operation contract.
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    /// Description: Describes the operator= operation contract.
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;
    /// Description: Describes the open operation contract.
    bool open(const std::string& path, std::string& outError);
    /// Description: Describes the close operation contract.
    void close();
    /// Description: Describes the is open operation contract.
    bool isOpen() const;
    /// Description: Describes the load symbol address operation contract.
    bool loadSymbolAddress(std::string_view name, std::uintptr_t& outSymbol,
                           std::string& outError) const;

private:
    /// Description: Describes the load raw symbol operation contract.
    bool loadRawSymbol(std::string_view name, std::uintptr_t& outSymbol,
                       std::string& outError) const;
    /// Description: Defines the Impl data or behavior contract.
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
} // namespace grav_platform
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
