/*
 * @file engine/src/config/SimulationOptionRegistryInternal.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#ifndef BLITZAR_SIM_SIMULATIONOPTIONREGISTRYINTERNAL_HPP
#define BLITZAR_SIM_SIMULATIONOPTIONREGISTRYINTERNAL_HPP
#include "config/SimulationArgsParse.hpp"
#include "config/SimulationOptionRegistry.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bltzr_config {
enum class OptionKind {
    Uint,
    Int,
    Float,
    Bool,
    String,
    PerformanceProfile,
    Solver,
    Integrator,
    OctreeCriterion,
    ClientParticleCap,
    TimeoutTriple,
};

struct SimulationOptionEntry {
    SimulationOptionGroup group;
    OptionKind kind;
    std::string_view cliName;
    std::string_view cliAlias;
    std::string_view iniName;
    std::string_view iniAlias;
    std::string_view envName;
    std::string_view usage;
    std::string_view aliasUsage;
    std::ptrdiff_t offset;
    double minValue;
    double maxValue;
    bool hasMin;
    bool hasMax;
};

extern const SimulationOptionEntry kSimulationOptions[];
extern const std::size_t kSimulationOptionCount;
bool matchesCli(const SimulationOptionEntry& entry, const std::string& key,
                SimulationOptionGroup group);
bool matchesIni(const SimulationOptionEntry& entry, const std::string& key);
bool matchesEnv(const SimulationOptionEntry& entry, const std::string& key);
bool applyEntry(const SimulationOptionEntry& entry, const std::string& value,
                SimulationConfig& config, std::ostream& warnings, std::string_view source,
                std::string_view optionName);
} // namespace bltzr_config
#endif // BLITZAR_SIM_SIMULATIONOPTIONREGISTRYINTERNAL_HPP
