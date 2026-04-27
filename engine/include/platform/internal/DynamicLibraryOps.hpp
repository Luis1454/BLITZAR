/*
 * @file engine/include/platform/internal/DynamicLibraryOps.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction interfaces for portable runtime services.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_DYNAMICLIBRARYOPS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_DYNAMICLIBRARYOPS_HPP_
#include <cstdint>
#include <string>
#include <string_view>

namespace grav_platform_detail {
typedef std::uintptr_t NativeLibraryHandle;
typedef std::uintptr_t NativeSymbolAddress;
bool openDynamicLibrary(const std::string& path, NativeLibraryHandle& outHandle,
                        std::string& outError);
void closeDynamicLibrary(NativeLibraryHandle& handle);
bool isDynamicLibraryOpen(NativeLibraryHandle handle);
bool loadDynamicSymbol(NativeLibraryHandle handle, std::string_view name,
                       NativeSymbolAddress& outSymbol, std::string& outError);
} // namespace grav_platform_detail
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_DYNAMICLIBRARYOPS_HPP_
