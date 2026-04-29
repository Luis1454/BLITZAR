/*
 * @file engine/src/platform/win/PlatformPathsWin.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction implementation for portable runtime services.
 */

#include "platform/win/PlatformPathsWin.hpp"

namespace bltzr_platform {
std::string executableName(std::string_view basename)
{
    return std::string(basename) + ".exe";
}

std::string_view serverDefaultExecutableName()
{
    return "blitzar-server.exe";
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
} // namespace bltzr_platform
