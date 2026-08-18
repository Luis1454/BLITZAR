/*
 * @file engine/config/registry/application/CfgApply.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Registry matching and dispatch for configuration values.
 */

#include "config/registry/runtime/CfgInternal.hpp"

#include <ostream>

namespace bltzr_config {

bool matchesCli(const SimulationOptionEntry& entry, const std::string& key,
                SimulationOptionGroup group)
{
    if (entry.group != group) {
        return false;
    }
    return key == entry.cliName || (!entry.cliAlias.empty() && key == entry.cliAlias);
}

bool matchesIni(const SimulationOptionEntry& entry, const std::string& key)
{
    return key == entry.iniName || (!entry.iniAlias.empty() && key == entry.iniAlias);
}

bool matchesEnv(const SimulationOptionEntry& entry, const std::string& key)
{
    return !entry.envName.empty() && key == entry.envName;
}

bool applyEntry(const SimulationOptionEntry& entry, const std::string& value,
                SimulationConfig& config, std::ostream& warnings, std::string_view source,
                std::string_view optionName)
{
    switch (entry.kind) {
    case OptionKind::Uint:
    case OptionKind::Int:
    case OptionKind::Float:
    case OptionKind::Bool:
    case OptionKind::String:
    case OptionKind::ClientParticleCap:
    case OptionKind::TimeoutTriple:
        return applyScalarEntry(entry, value, config, warnings, source, optionName);
    case OptionKind::PerformanceProfile:
    case OptionKind::Solver:
    case OptionKind::Integrator:
    case OptionKind::OctreeCriterion:
    case OptionKind::TreePmModel:
    case OptionKind::TreePmLayout:
    case OptionKind::TreePmPrecision:
    case OptionKind::TreePmAssignment:
    case OptionKind::TreePmPreset:
        return applyNormalizedEntry(entry, value, config, warnings, source, optionName);
    }
    return false;
}

template <typename Matcher>
static bool applyMatchingEntry(Matcher matcher, const std::string& key, const std::string& value,
                               SimulationConfig& config, std::ostream& warnings,
                               std::string_view source)
{
    for (std::size_t rangeIndex = 0; rangeIndex < kSimulationOptionRangeCount; ++rangeIndex) {
        const SimulationOptionRange& range = kSimulationOptionRanges[rangeIndex];
        for (std::size_t entryIndex = 0; entryIndex < range.count; ++entryIndex) {
            const SimulationOptionEntry& entry = *(&range.first + entryIndex);
            if (!matcher(entry, key)) {
                continue;
            }
            return applyEntry(entry, value, config, warnings, source, key);
        }
    }
    return false;
}

bool applyCliOption(SimulationOptionGroup group, const std::string& key, const std::string& value,
                    SimulationConfig& config, std::ostream& warnings)
{
    return applyMatchingEntry(
        [group](const SimulationOptionEntry& entry, const std::string& candidate) {
            return matchesCli(entry, candidate, group);
        },
        key, value, config, warnings, "[args]");
}

bool applyIniOption(const std::string& key, const std::string& value, SimulationConfig& config,
                    std::ostream& warnings)
{
    return applyMatchingEntry(matchesIni, key, value, config, warnings, "[config]");
}

bool applyEnvOption(const std::string& key, const std::string& value, SimulationConfig& config,
                    std::ostream& warnings)
{
    return applyMatchingEntry(matchesEnv, key, value, config, warnings, "[env]");
}

} // namespace bltzr_config
