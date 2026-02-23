#ifndef GRAVITY_PLATFORM_INTERNAL_DYNAMICLIBRARYOPS_HPP
#define GRAVITY_PLATFORM_INTERNAL_DYNAMICLIBRARYOPS_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace sim::platform::detail {

using NativeLibraryHandle = std::uintptr_t;
using NativeSymbolAddress = std::uintptr_t;

bool openDynamicLibrary(const std::string &path, NativeLibraryHandle &outHandle, std::string &outError);
void closeDynamicLibrary(NativeLibraryHandle &handle);
bool isDynamicLibraryOpen(NativeLibraryHandle handle);
bool loadDynamicSymbol(
    NativeLibraryHandle handle,
    std::string_view name,
    NativeSymbolAddress &outSymbol,
    std::string &outError);

} // namespace sim::platform::detail

#endif // GRAVITY_PLATFORM_INTERNAL_DYNAMICLIBRARYOPS_HPP
