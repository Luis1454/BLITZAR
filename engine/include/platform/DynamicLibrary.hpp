/*
 * @file engine/include/platform/DynamicLibrary.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction interfaces for portable runtime services.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
#define BLITZAR_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
#include "platform/PlatformErrors.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace bltzr_platform {
class DynamicLibrary {
public:
    DynamicLibrary();
    ~DynamicLibrary();
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;
    bool open(const std::string& path, std::string& outError);
    void close();
    bool isOpen() const;
    bool loadSymbolAddress(std::string_view name, std::uintptr_t& outSymbol,
                           std::string& outError) const;

private:
    bool loadRawSymbol(std::string_view name, std::uintptr_t& outSymbol,
                       std::string& outError) const;
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
} // namespace bltzr_platform
#endif // BLITZAR_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
