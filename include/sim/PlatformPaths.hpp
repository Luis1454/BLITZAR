#ifndef GRAVITY_SIM_PLATFORMPATHS_HPP
#define GRAVITY_SIM_PLATFORMPATHS_HPP

#include <ctime>
#include <string>
#include <string_view>
#include <vector>

namespace sim::platform {

std::string executableName(std::string_view basename);
std::string_view backendDefaultExecutableName();
std::vector<std::string> sharedLibraryCandidates(std::string_view stem);
std::tm localTime(std::time_t nowTime);

} // namespace sim::platform

#endif // GRAVITY_SIM_PLATFORMPATHS_HPP
