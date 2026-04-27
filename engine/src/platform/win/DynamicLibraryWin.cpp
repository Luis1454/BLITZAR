// File: engine/src/platform/win/DynamicLibraryWin.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/win/DynamicLibraryWin.hpp"
#include <windows.h>
namespace grav_platform_detail {
bool openDynamicLibrary(const std::string& path, NativeLibraryHandle& outHandle,
                        std::string& outError)
{
    outError.clear();
    const HMODULE module = LoadLibraryA(path.c_str());
    outHandle = reinterpret_cast<NativeLibraryHandle>(module);
    return module != nullptr;
}
void closeDynamicLibrary(NativeLibraryHandle& handle)
{
    if (handle == 0u) {
        return;
    }
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
    handle = 0u;
}
bool isDynamicLibraryOpen(NativeLibraryHandle handle)
{
    return handle != 0u;
}
bool loadDynamicSymbol(NativeLibraryHandle handle, std::string_view name,
                       NativeSymbolAddress& outSymbol, std::string& outError)
{
    outError.clear();
    outSymbol = 0u;
    if (handle == 0u || name.empty()) {
        return false;
    }
    const std::string symbolName(name);
    FARPROC raw = GetProcAddress(reinterpret_cast<HMODULE>(handle), symbolName.c_str());
    if (raw == nullptr)
        return false;
    outSymbol = reinterpret_cast<NativeSymbolAddress>(raw);
    return true;
}
} // namespace grav_platform_detail
