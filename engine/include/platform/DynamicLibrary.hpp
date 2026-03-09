#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_

#include "platform/PlatformErrors.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace grav_platform {

class DynamicLibrary {
    public:
        DynamicLibrary();
        ~DynamicLibrary();

        DynamicLibrary(const DynamicLibrary &) = delete;
        DynamicLibrary &operator=(const DynamicLibrary &) = delete;

        DynamicLibrary(DynamicLibrary &&other) noexcept;
        DynamicLibrary &operator=(DynamicLibrary &&other) noexcept;

        bool open(const std::string &path, std::string &outError);

        void close();

        bool isOpen() const;
        bool loadSymbolAddress(std::string_view name, std::uintptr_t &outSymbol, std::string &outError) const;

    private:
        bool loadRawSymbol(std::string_view name, std::uintptr_t &outSymbol, std::string &outError) const;
        struct Impl;
        std::unique_ptr<Impl> _impl;
};

} // namespace grav_platform



#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_DYNAMICLIBRARY_HPP_
