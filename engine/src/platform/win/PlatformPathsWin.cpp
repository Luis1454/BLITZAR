#include "platform/win/PlatformPathsWin.hpp"

namespace grav_platform {

std::string executableName(std::string_view basename)
{
    return std::string(basename) + ".exe";
}

std::string_view serverDefaultExecutableName()
{
    return "aster-server.exe";
}

std::vector<std::string> sharedLibraryCandidates(std::string_view stem)
{
    return {std::string(stem) + ".dll"};
}

std::tm localTime(std::time_t nowTime)
{
    std::tm tm{};
    localtime_s(&tm, &nowTime);
    return tm;
}

} // namespace grav_platform
