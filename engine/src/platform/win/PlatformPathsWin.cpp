// File: engine/src/platform/win/PlatformPathsWin.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/win/PlatformPathsWin.hpp"

namespace grav_platform {
/// Description: Executes the executableName operation.
std::string executableName(std::string_view basename)
{
    return std::string(basename) + ".exe";
}

/// Description: Executes the serverDefaultExecutableName operation.
std::string_view serverDefaultExecutableName()
{
    return "blitzar-server.exe";
}

/// Description: Executes the sharedLibraryCandidates operation.
std::vector<std::string> sharedLibraryCandidates(std::string_view stem)
{
    return {std::string(stem) + ".dll"};
}

/// Description: Executes the localTime operation.
std::tm localTime(std::time_t nowTime)
{
    std::tm tm{};
    localtime_s(&tm, &nowTime);
    return tm;
}
} // namespace grav_platform
