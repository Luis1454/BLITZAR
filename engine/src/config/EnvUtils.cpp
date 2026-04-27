// File: engine/src/config/EnvUtils.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/EnvUtils.hpp"
#include "config/SimulationArgsParse.hpp"

namespace grav_env {
/// Description: Executes the parseBool operation.
bool parseBool(std::string_view value, bool fallback)
{
    bool parsed = fallback;
    if (SimulationArgsParse::parseBool(std::string(value), parsed)) {
        return parsed;
    }
    return fallback;
}

/// Description: Executes the getBool operation.
bool getBool(std::string_view name, bool fallback)
{
    const auto value = get(name);
    if (!value.has_value()) {
        return fallback;
    }
    return parseBool(*value, fallback);
}

/// Description: Executes the getNumber operation.
template <typename NumberType> bool getNumber(std::string_view name, NumberType& out)
{
    const auto value = get(name);
    if (!value.has_value()) {
        return false;
    }
    return grav_text::parseNumber(*value, out);
}

/// Description: Describes the get number<short> operation contract.
template bool getNumber<short>(std::string_view name, short& out);
/// Description: Describes the short> operation contract.
template bool getNumber<unsigned short>(std::string_view name, unsigned short& out);
/// Description: Describes the get number<int> operation contract.
template bool getNumber<int>(std::string_view name, int& out);
/// Description: Describes the int> operation contract.
template bool getNumber<unsigned int>(std::string_view name, unsigned int& out);
/// Description: Describes the get number<long> operation contract.
template bool getNumber<long>(std::string_view name, long& out);
/// Description: Describes the long> operation contract.
template bool getNumber<unsigned long>(std::string_view name, unsigned long& out);
/// Description: Describes the long> operation contract.
template bool getNumber<long long>(std::string_view name, long long& out);
/// Description: Describes the long> operation contract.
template bool getNumber<unsigned long long>(std::string_view name, unsigned long long& out);
/// Description: Describes the get number<float> operation contract.
template bool getNumber<float>(std::string_view name, float& out);
/// Description: Describes the get number<double> operation contract.
template bool getNumber<double>(std::string_view name, double& out);
} // namespace grav_env
