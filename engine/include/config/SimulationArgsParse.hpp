// File: engine/include/config/SimulationArgsParse.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/// Description: Defines the SimulationArgsParse data or behavior contract.
class SimulationArgsParse final {
public:
    /// Description: Describes the to lower operation contract.
    static std::string toLower(std::string value);
    /// Description: Describes the parse bool operation contract.
    static bool parseBool(const std::string& value, bool& out);
    /// Description: Describes the parse uint operation contract.
    static bool parseUint(const std::string& value, std::uint32_t& out);
    /// Description: Describes the parse int operation contract.
    static bool parseInt(const std::string& value, int& out);
    /// Description: Describes the parse float operation contract.
    static bool parseFloat(const std::string& value, float& out);
    /// Description: Describes the split option operation contract.
    static bool splitOption(const std::string& raw, std::string& key, std::string& value);
    /// Description: Describes the read value operation contract.
    static bool readValue(const std::vector<std::string_view>& args, std::size_t& index,
                          const std::string& inlined, std::string& outValue);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
