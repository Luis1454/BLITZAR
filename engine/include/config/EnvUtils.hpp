// File: engine/include/config/EnvUtils.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

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
/// Description: Executes the get operation.
std::optional<std::string> get(std::string_view name);
/// Description: Executes the parseBool operation.
bool parseBool(std::string_view value, bool fallback);
/// Description: Executes the getBool operation.
bool getBool(std::string_view name, bool fallback);
/// Description: Executes the getNumber operation.
template <typename NumberType> bool getNumber(std::string_view name, NumberType& out);
/// Description: Describes the get number<short> operation contract.
extern template bool getNumber<short>(std::string_view name, short& out);
/// Description: Describes the short> operation contract.
extern template bool getNumber<unsigned short>(std::string_view name, unsigned short& out);
/// Description: Describes the get number<int> operation contract.
extern template bool getNumber<int>(std::string_view name, int& out);
/// Description: Describes the int> operation contract.
extern template bool getNumber<unsigned int>(std::string_view name, unsigned int& out);
/// Description: Describes the get number<long> operation contract.
extern template bool getNumber<long>(std::string_view name, long& out);
/// Description: Describes the long> operation contract.
extern template bool getNumber<unsigned long>(std::string_view name, unsigned long& out);
/// Description: Describes the long> operation contract.
extern template bool getNumber<long long>(std::string_view name, long long& out);
/// Description: Describes the long> operation contract.
extern template bool getNumber<unsigned long long>(std::string_view name, unsigned long long& out);
/// Description: Describes the get number<float> operation contract.
extern template bool getNumber<float>(std::string_view name, float& out);
/// Description: Describes the get number<double> operation contract.
extern template bool getNumber<double>(std::string_view name, double& out);
} // namespace grav_env
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_ENVUTILS_HPP_
