#include "sim/DynamicLibrary.hpp"
#include "sim/PlatformErrors.hpp"

#include "src/cpp/platform/internal/DynamicLibraryOps.hpp"

#include <exception>
#include <string_view>
#include <utility>

namespace sim::platform {

struct DynamicLibrary::Impl {
    detail::NativeLibraryHandle handle = 0u;
};

DynamicLibrary::DynamicLibrary()
    : _impl(std::make_unique<Impl>())
{
}

DynamicLibrary::~DynamicLibrary() = default;

DynamicLibrary::DynamicLibrary(DynamicLibrary &&other) noexcept
    : _impl(std::move(other._impl))
{
}

DynamicLibrary &DynamicLibrary::operator=(DynamicLibrary &&other) noexcept
{
    if (this != &other) {
        _impl = std::move(other._impl);
    }
    return *this;
}

bool DynamicLibrary::open(const std::string &path, std::string &outError)
{
    try {
        if (!_impl) {
            _impl = std::make_unique<Impl>();
        }
        close();
        if (!detail::openDynamicLibrary(path, _impl->handle, outError)) {
            if (outError.empty()) {
                outError = errors::kDynamicLibraryLoadFailed;
            }
            return false;
        }
        return true;
    } catch (const std::exception &ex) {
        outError = ex.what();
        return false;
    } catch (...) {
        outError = errors::kUnknownException;
        return false;
    }
}

void DynamicLibrary::close()
{
    if (_impl) {
        detail::closeDynamicLibrary(_impl->handle);
    }
}

bool DynamicLibrary::isOpen() const
{
    return _impl && detail::isDynamicLibraryOpen(_impl->handle);
}

bool DynamicLibrary::loadRawSymbol(std::string_view name, std::uintptr_t &outSymbol, std::string &outError) const
{
    try {
        outSymbol = 0u;
        if (!_impl || !detail::isDynamicLibraryOpen(_impl->handle) || name.empty()) {
            outError = errors::kInvalidLibraryHandleOrSymbol;
            return false;
        }
        if (!detail::loadDynamicSymbol(_impl->handle, name, outSymbol, outError)) {
            if (outError.empty()) {
                outError = std::string(errors::kMissingSymbolPrefix) + std::string(name);
            }
            return false;
        }
        return true;
    } catch (const std::exception &ex) {
        outError = ex.what();
        return false;
    } catch (...) {
        outError = errors::kUnknownException;
        return false;
    }
}

} // namespace sim::platform
