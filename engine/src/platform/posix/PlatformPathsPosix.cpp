#include "platform/posix/PlatformPathsPosix.hpp"

#ifndef GRAVITY_PLATFORM_DYLIB_EXT
#define GRAVITY_PLATFORM_DYLIB_EXT ".so"
#endif

namespace grav_platform {

std::string executableName(std::string_view basename)
{
    return std::string(basename);
}

std::string_view backendDefaultExecutableName()
{
    return "myAppBackend";
}

std::vector<std::string> sharedLibraryCandidates(std::string_view stem)
{
    return {"lib" + std::string(stem) + GRAVITY_PLATFORM_DYLIB_EXT, std::string(stem) + GRAVITY_PLATFORM_DYLIB_EXT};
}

std::tm localTime(std::time_t nowTime)
{
    std::tm tm{};
    localtime_r(&nowTime, &tm);
    return tm;
}

} // namespace grav_platform
