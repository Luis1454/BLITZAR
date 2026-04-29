/*
 * @file engine/src/platform/posix/PlatformPathsPosix.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction implementation for portable runtime services.
 */

#include "platform/posix/PlatformPathsPosix.hpp"

namespace bltzr_platform {
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
} // namespace bltzr_platform
