#include "platform/common/DynamicLibraryCommon.hpp"
#include "platform/PlatformErrors.hpp"
#include "platform/internal/DynamicLibraryOps.hpp"
#include <exception>
#include <string_view>
#include <utility>
namespace grav_platform {
struct DynamicLibrary::Impl {
    grav_platform_detail::NativeLibraryHandle handle = 0u;
};
DynamicLibrary::DynamicLibrary() : _impl(std::make_unique<Impl>())
{
}
DynamicLibrary::~DynamicLibrary() = default;
DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept : _impl(std::move(other._impl))
{
}
DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept
{
    if (this != &other)
        _impl = std::move(other._impl);
    return *this;
}
bool DynamicLibrary::open(const std::string& path, std::string& outError)
{
    try {
        if (!_impl)
            _impl = std::make_unique<Impl>();
        close();
        if (!grav_platform_detail::openDynamicLibrary(path, _impl->handle, outError)) {
            if (outError.empty()) {
                outError = grav_platform_errors::kDynamicLibraryLoadFailed;
            }
            return false;
        }
        return true;
    }
    catch (const std::exception& ex) {
        outError = ex.what();
        return false;
    }
    catch (...) {
        outError = grav_platform_errors::kUnknownException;
        return false;
    }
}
void DynamicLibrary::close()
{
    if (_impl) {
        grav_platform_detail::closeDynamicLibrary(_impl->handle);
    }
}
bool DynamicLibrary::isOpen() const
{
    return _impl && grav_platform_detail::isDynamicLibraryOpen(_impl->handle);
}
bool DynamicLibrary::loadSymbolAddress(std::string_view name, std::uintptr_t& outSymbol,
                                       std::string& outError) const
{
    if (name.empty() || !isOpen()) {
        outError = grav_platform_errors::kInvalidLibraryHandleOrSymbol;
        outSymbol = 0u;
        return false;
    }
    return loadRawSymbol(name, outSymbol, outError);
}
bool DynamicLibrary::loadRawSymbol(std::string_view name, std::uintptr_t& outSymbol,
                                   std::string& outError) const
{
    try {
        outSymbol = 0u;
        if (!_impl || !grav_platform_detail::isDynamicLibraryOpen(_impl->handle) || name.empty()) {
            outError = grav_platform_errors::kInvalidLibraryHandleOrSymbol;
            return false;
        }
        if (!grav_platform_detail::loadDynamicSymbol(_impl->handle, name, outSymbol, outError)) {
            if (outError.empty()) {
                outError =
                    std::string(grav_platform_errors::kMissingSymbolPrefix) + std::string(name);
            }
            return false;
        }
        return true;
    }
    catch (const std::exception& ex) {
        outError = ex.what();
        return false;
    }
    catch (...) {
        outError = grav_platform_errors::kUnknownException;
        return false;
    }
}
} // namespace grav_platform
