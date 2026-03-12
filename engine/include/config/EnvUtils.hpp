#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_ENVUTILS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_ENVUTILS_HPP_

#include "config/TextParse.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace grav_env {

std::optional<std::string> get(std::string_view name);
bool parseBool(std::string_view value, bool fallback);
bool getBool(std::string_view name, bool fallback);

template <typename NumberType>
bool getNumber(std::string_view name, NumberType &out);

extern template bool getNumber<short>(std::string_view name, short &out);
extern template bool getNumber<unsigned short>(std::string_view name, unsigned short &out);
extern template bool getNumber<int>(std::string_view name, int &out);
extern template bool getNumber<unsigned int>(std::string_view name, unsigned int &out);
extern template bool getNumber<long>(std::string_view name, long &out);
extern template bool getNumber<unsigned long>(std::string_view name, unsigned long &out);
extern template bool getNumber<long long>(std::string_view name, long long &out);
extern template bool getNumber<unsigned long long>(std::string_view name, unsigned long long &out);
extern template bool getNumber<float>(std::string_view name, float &out);
extern template bool getNumber<double>(std::string_view name, double &out);

} // namespace grav_env



#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_ENVUTILS_HPP_
