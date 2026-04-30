/*
 * @file engine/include/config/SimulationArgsParse.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/*
 * @brief Defines the simulation args parse type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class SimulationArgsParse final {
public:
    /*
     * @brief Documents the to lower operation contract.
     * @param value Input value used by this contract.
     * @return std::string value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static std::string toLower(std::string value);
    /*
     * @brief Documents the parse bool operation contract.
     * @param value Input value used by this contract.
     * @param out Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool parseBool(const std::string& value, bool& out);
    /*
     * @brief Documents the parse uint operation contract.
     * @param value Input value used by this contract.
     * @param out Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool parseUint(const std::string& value, std::uint32_t& out);
    /*
     * @brief Documents the parse int operation contract.
     * @param value Input value used by this contract.
     * @param out Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool parseInt(const std::string& value, int& out);
    /*
     * @brief Documents the parse float operation contract.
     * @param value Input value used by this contract.
     * @param out Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool parseFloat(const std::string& value, float& out);
    /*
     * @brief Documents the split option operation contract.
     * @param raw Input value used by this contract.
     * @param key Input value used by this contract.
     * @param value Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool splitOption(const std::string& raw, std::string& key, std::string& value);
    /*
     * @brief Documents the read value operation contract.
     * @param args Input value used by this contract.
     * @param index Input value used by this contract.
     * @param inlined Input value used by this contract.
     * @param outValue Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool readValue(const std::vector<std::string_view>& args, std::size_t& index,
                          const std::string& inlined, std::string& outValue);
};
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONARGSPARSE_HPP_
