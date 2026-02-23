#ifndef GRAVITY_SIM_ENVUTILS_HPP
#define GRAVITY_SIM_ENVUTILS_HPP

#include "sim/TextParse.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace sim::env {

inline std::optional<std::string> get(std::string_view name)
{
    const std::string key(name);
    const auto rawValue = std::getenv(key.c_str());
    if (rawValue == nullptr) {
        return std::nullopt;
    }
    return std::string(rawValue);
}

inline bool parseBool(std::string_view value, bool fallback)
{
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        return false;
    }
    return fallback;
}

inline bool getBool(std::string_view name, bool fallback)
{
    const auto value = get(name);
    if (!value.has_value()) {
        return fallback;
    }
    return parseBool(*value, fallback);
}

template <typename NumberType>
inline bool getNumber(std::string_view name, NumberType &out)
{
    const auto value = get(name);
    if (!value.has_value()) {
        return false;
    }
    return sim::text::parseNumber(*value, out);
}

} // namespace sim::env

#endif // GRAVITY_SIM_ENVUTILS_HPP
