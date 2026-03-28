#include "platform/posix/DynamicLibraryPosix.hpp"
#include <dlfcn.h>
namespace grav_platform_detail {
bool openDynamicLibrary(const std::string& path, NativeLibraryHandle& outHandle,
                        std::string& outError)
{
    outError.clear();
    void* rawHandle = dlopen(path.c_str(), RTLD_NOW);
    outHandle = reinterpret_cast<NativeLibraryHandle>(rawHandle);
    if (rawHandle == nullptr) {
        const char* dlErr = dlerror();
        if (dlErr != nullptr)
            outError = dlErr;
        return false;
    }
    return true;
}
void closeDynamicLibrary(NativeLibraryHandle& handle)
{
    if (handle == 0u) {
        return;
    }
    dlclose(reinterpret_cast<void*>(handle));
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
    void* raw = dlsym(reinterpret_cast<void*>(handle), symbolName.c_str());
    if (raw == nullptr) {
        const char* dlErr = dlerror();
        if (dlErr != nullptr)
            outError = dlErr;
        return false;
    }
    outSymbol = reinterpret_cast<NativeSymbolAddress>(raw);
    return true;
}
} // namespace grav_platform_detail
