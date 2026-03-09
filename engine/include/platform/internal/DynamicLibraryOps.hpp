#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace grav_platform_detail {

typedef std::uintptr_t NativeLibraryHandle;
typedef std::uintptr_t NativeSymbolAddress;
bool openDynamicLibrary(const std::string &path, NativeLibraryHandle &outHandle, std::string &outError);
void closeDynamicLibrary(NativeLibraryHandle &handle);
bool isDynamicLibraryOpen(NativeLibraryHandle handle);
bool loadDynamicSymbol(
    NativeLibraryHandle handle,
    std::string_view name,
    NativeSymbolAddress &outSymbol,
    std::string &outError);

} // namespace grav_platform_detail

