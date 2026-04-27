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
    /// Description: Executes the DynamicLibrary operation.
    DynamicLibrary();
    /// Description: Releases resources owned by DynamicLibrary.
    ~DynamicLibrary();
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;
    /// Description: Executes the open operation.
    bool open(const std::string& path, std::string& outError);
    /// Description: Executes the close operation.
    void close();
    /// Description: Executes the isOpen operation.
    bool isOpen() const;
    bool loadSymbolAddress(std::string_view name, std::uintptr_t& outSymbol,
                           std::string& outError) const;

private:
    bool loadRawSymbol(std::string_view name, std::uintptr_t& outSymbol,
                       std::string& outError) const;
    /// Description: Defines the Impl data or behavior contract.
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
} // namespace grav_platform
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
