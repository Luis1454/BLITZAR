#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
#include <ctime>
#include <string>
#include <string_view>
#include <vector>
namespace grav_platform {
std::string executableName(std::string_view basename);
std::string_view serverDefaultExecutableName();
std::vector<std::string> sharedLibraryCandidates(std::string_view stem);
std::tm localTime(std::time_t nowTime);
} // namespace grav_platform
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
