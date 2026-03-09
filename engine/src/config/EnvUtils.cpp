#include "config/EnvUtils.hpp"
#include "config/SimulationArgsParse.hpp"

namespace grav_env {

std::optional<std::string> get(std::string_view name)
{
    const std::string key(name);
#ifdef _WIN32
    char *rawValue = nullptr;
    std::size_t rawSize = 0;
    if (_dupenv_s(&rawValue, &rawSize, key.c_str()) != 0 || rawValue == nullptr) {
        return std::nullopt;
    }
    std::string value(rawValue);
    std::free(rawValue);
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
#else
    const auto rawValue = std::getenv(key.c_str());
    if (rawValue == nullptr) {
        return std::nullopt;
    }
    return std::string(rawValue);
#endif
}

bool parseBool(std::string_view value, bool fallback)
{
    bool parsed = fallback;
    if (SimulationArgsParse::parseBool(std::string(value), parsed)) {
        return parsed;
    }
    return fallback;
}

bool getBool(std::string_view name, bool fallback)
{
    const auto value = get(name);
    if (!value.has_value()) {
        return fallback;
    }
    return parseBool(*value, fallback);
}

template <typename NumberType>
bool getNumber(std::string_view name, NumberType &out)
{
    const auto value = get(name);
    if (!value.has_value()) {
        return false;
    }
    return grav_text::parseNumber(*value, out);
}

template bool getNumber<short>(std::string_view name, short &out);
template bool getNumber<unsigned short>(std::string_view name, unsigned short &out);
template bool getNumber<int>(std::string_view name, int &out);
template bool getNumber<unsigned int>(std::string_view name, unsigned int &out);
template bool getNumber<long>(std::string_view name, long &out);
template bool getNumber<unsigned long>(std::string_view name, unsigned long &out);
template bool getNumber<long long>(std::string_view name, long long &out);
template bool getNumber<unsigned long long>(std::string_view name, unsigned long long &out);
template bool getNumber<float>(std::string_view name, float &out);
template bool getNumber<double>(std::string_view name, double &out);

} // namespace grav_env
