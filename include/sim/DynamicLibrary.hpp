#ifndef GRAVITY_SIM_DYNAMICLIBRARY_HPP
#define GRAVITY_SIM_DYNAMICLIBRARY_HPP

#include "sim/PlatformErrors.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace sim::platform {

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

        template <typename FnType>
        FnType loadSymbol(std::string_view name, std::string &outError) const;

    private:
        bool loadRawSymbol(std::string_view name, std::uintptr_t &outSymbol, std::string &outError) const;
        struct Impl;
        std::unique_ptr<Impl> _impl;
};

template <typename FnType>
FnType DynamicLibrary::loadSymbol(std::string_view name, std::string &outError) const
{
    if (name.empty() || !isOpen()) {
        outError = errors::kInvalidLibraryHandleOrSymbol;
        return nullptr;
    }
    std::uintptr_t rawSymbol = 0u;
    if (!loadRawSymbol(name, rawSymbol, outError)) {
        return nullptr;
    }
    return reinterpret_cast<FnType>(rawSymbol);
}

} // namespace sim::platform

#endif // GRAVITY_SIM_DYNAMICLIBRARY_HPP
