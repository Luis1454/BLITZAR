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
    /// Description: Executes the toLower operation.
    static std::string toLower(std::string value);
    /// Description: Executes the parseBool operation.
    static bool parseBool(const std::string& value, bool& out);
    /// Description: Executes the parseUint operation.
    static bool parseUint(const std::string& value, std::uint32_t& out);
    /// Description: Executes the parseInt operation.
    static bool parseInt(const std::string& value, int& out);
    /// Description: Executes the parseFloat operation.
    static bool parseFloat(const std::string& value, float& out);
    /// Description: Executes the splitOption operation.
    static bool splitOption(const std::string& raw, std::string& key, std::string& value);
    static bool readValue(const std::vector<std::string_view>& args, std::size_t& index,
                          const std::string& inlined, std::string& outValue);
};
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
