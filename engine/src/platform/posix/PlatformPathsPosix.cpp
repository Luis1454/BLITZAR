// File: engine/src/platform/posix/PlatformPathsPosix.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/posix/PlatformPathsPosix.hpp"
namespace grav_platform {
static constexpr std::string_view kPlatformDylibExtension = ".so";
/// Description: Executes the executableName operation.
std::string executableName(std::string_view basename)
{
    return std::string(basename);
}
/// Description: Executes the serverDefaultExecutableName operation.
std::string_view serverDefaultExecutableName()
{
    return "blitzar-server";
}
/// Description: Executes the sharedLibraryCandidates operation.
std::vector<std::string> sharedLibraryCandidates(std::string_view stem)
{
    return {"lib" + std::string(stem) + std::string(kPlatformDylibExtension),
            std::string(stem) + std::string(kPlatformDylibExtension)};
}
/// Description: Executes the localTime operation.
std::tm localTime(std::time_t nowTime)
{
    std::tm tm{};
    /// Description: Executes the localtime_r operation.
    localtime_r(&nowTime, &tm);
    return tm;
}
} // namespace grav_platform
