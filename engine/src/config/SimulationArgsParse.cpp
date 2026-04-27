/*
 * @file engine/src/config/SimulationArgsParse.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationArgsParse.hpp"
#include "config/TextParse.hpp"
#include <algorithm>
#include <cctype>
#include <limits>

/*
 * @brief Documents the to lower operation contract.
 * @param value Input value used by this contract.
 * @return std::string SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string SimulationArgsParse::toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

/*
 * @brief Documents the parse bool operation contract.
 * @param value Input value used by this contract.
 * @param out Input value used by this contract.
 * @return bool SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsParse::parseBool(const std::string& value, bool& out)
{
    const std::string normalized = SimulationArgsParse::toLower(value);
    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        out = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        out = false;
        return true;
    }
    return false;
}

/*
 * @brief Documents the parse uint operation contract.
 * @param value Input value used by this contract.
 * @param out Input value used by this contract.
 * @return bool SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsParse::parseUint(const std::string& value, std::uint32_t& out)
{
    std::uint64_t parsed = 0;
    if (!grav_text::parseNumber(value, parsed)) {
        return false;
    }
    if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    out = static_cast<std::uint32_t>(parsed);
    return true;
}

/*
 * @brief Documents the parse int operation contract.
 * @param value Input value used by this contract.
 * @param out Input value used by this contract.
 * @return bool SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsParse::parseInt(const std::string& value, int& out)
{
    long long parsed = 0;
    if (!grav_text::parseNumber(value, parsed)) {
        return false;
    }
    if (parsed < static_cast<long long>(std::numeric_limits<int>::min()) ||
        parsed > static_cast<long long>(std::numeric_limits<int>::max())) {
        return false;
    }
    out = static_cast<int>(parsed);
    return true;
}

/*
 * @brief Documents the parse float operation contract.
 * @param value Input value used by this contract.
 * @param out Input value used by this contract.
 * @return bool SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsParse::parseFloat(const std::string& value, float& out)
{
    return grav_text::parseNumber(value, out);
}

/*
 * @brief Documents the split option operation contract.
 * @param raw Input value used by this contract.
 * @param key Input value used by this contract.
 * @param value Input value used by this contract.
 * @return bool SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsParse::splitOption(const std::string& raw, std::string& key, std::string& value)
{
    if (raw.rfind("--", 0) != 0) {
        return false;
    }
    const std::size_t eq = raw.find('=');
    if (eq == std::string::npos) {
        key = raw;
        value.clear();
        return true;
    }
    key = raw.substr(0, eq);
    value = raw.substr(eq + 1);
    return true;
}

/*
 * @brief Documents the read value operation contract.
 * @param args Input value used by this contract.
 * @param index Input value used by this contract.
 * @param inlined Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool SimulationArgsParse:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsParse::readValue(const std::vector<std::string_view>& args, std::size_t& index,
                                    const std::string& inlined, std::string& outValue)
{
    if (!inlined.empty()) {
        outValue = inlined;
        return true;
    }
    if (index + 1 >= args.size() || args[index + 1].empty()) {
        return false;
    }
    outValue = std::string(args[++index]);
    return true;
}
