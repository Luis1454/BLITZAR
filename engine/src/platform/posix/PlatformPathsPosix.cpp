// File: engine/src/platform/posix/PlatformPathsPosix.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/posix/PlatformPathsPosix.hpp"
namespace grav_platform {
static constexpr std::string_view kPlatformDylibExtension = ".so";
std::string executableName(std::string_view basename)
{
    return std::string(basename);
}
std::string_view serverDefaultExecutableName()
{
    return "blitzar-server";
}
std::vector<std::string> sharedLibraryCandidates(std::string_view stem)
{
    return {"lib" + std::string(stem) + std::string(kPlatformDylibExtension),
            std::string(stem) + std::string(kPlatformDylibExtension)};
}
std::tm localTime(std::time_t nowTime)
{
    std::tm tm{};
    localtime_r(&nowTime, &tm);
    return tm;
}
} // namespace grav_platform
