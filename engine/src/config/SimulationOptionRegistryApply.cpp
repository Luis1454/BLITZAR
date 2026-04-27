/*
 * @file engine/src/config/SimulationOptionRegistryApply.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "SimulationOptionRegistryInternal.hpp"

#include "config/SimulationConfig.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "protocol/ServerProtocol.hpp"

#include <ostream>

namespace grav_config {

template <typename ValueType>
static ValueType& memberAt(SimulationConfig& config, std::ptrdiff_t offset)
{
    return *reinterpret_cast<ValueType*>(reinterpret_cast<char*>(&config) + offset);
}

static void emitInvalid(std::ostream& warnings, std::string_view source,
                        std::string_view optionName, const std::string& value)
{
    warnings << source << " invalid " << optionName << ": " << value << "\n";
}

static void emitClamped(std::ostream& warnings, std::string_view source,
                        std::string_view optionName, std::uint32_t requested, std::uint32_t clamped)
{
    warnings << source << ' ' << optionName << " clamped to supported range [2, "
             << grav_protocol::kSnapshotMaxPoints << "]: " << requested << " -> " << clamped
             << "\n";
}

static std::uint32_t clampClientParticleCap(std::uint32_t requested)
{
    if (requested > grav_protocol::kSnapshotMaxPoints) {
        return grav_protocol::kSnapshotMaxPoints;
    }
    return requested < 2u ? 2u : requested;
}

static void markCustomPerformanceProfile(const SimulationOptionEntry& entry,
                                         SimulationConfig& config)
{
    if (grav_config::isPerformanceManagedField(entry.iniName)) {
        config.performanceProfile = std::string(grav_config::kPerformanceProfileCustom);
    }
}

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
    case OptionKind::Uint: {
        std::uint32_t parsed = memberAt<std::uint32_t>(config, entry.offset);
        if (!SimulationArgsParse::parseUint(value, parsed) ||
            (entry.hasMin && parsed < static_cast<std::uint32_t>(entry.minValue)) ||
            (entry.hasMax && parsed > static_cast<std::uint32_t>(entry.maxValue))) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        memberAt<std::uint32_t>(config, entry.offset) = parsed;
        markCustomPerformanceProfile(entry, config);
        return true;
    }
    case OptionKind::Int: {
        int parsed = memberAt<int>(config, entry.offset);
        if (!SimulationArgsParse::parseInt(value, parsed) ||
            (entry.hasMin && parsed < static_cast<int>(entry.minValue)) ||
            (entry.hasMax && parsed > static_cast<int>(entry.maxValue))) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        memberAt<int>(config, entry.offset) = parsed;
        return true;
    }
    case OptionKind::Float: {
        float parsed = memberAt<float>(config, entry.offset);
        if (!SimulationArgsParse::parseFloat(value, parsed) ||
            (entry.hasMin && parsed < static_cast<float>(entry.minValue)) ||
            (entry.hasMax && parsed > static_cast<float>(entry.maxValue))) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        memberAt<float>(config, entry.offset) = parsed;
        return true;
    }
    case OptionKind::Bool: {
        bool parsed = memberAt<bool>(config, entry.offset);
        if (!SimulationArgsParse::parseBool(value, parsed)) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        memberAt<bool>(config, entry.offset) = parsed;
        return true;
    }
    case OptionKind::String:
        memberAt<std::string>(config, entry.offset) = value;
        return true;
    case OptionKind::PerformanceProfile: {
        std::string canonical;
        if (!grav_config::normalizePerformanceProfile(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: interactive|balanced|quality|custom)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        grav_config::applyPerformanceProfile(config);
        return true;
    }
    case OptionKind::Solver: {
        std::string canonical;
        if (!grav_modes::normalizeSolver(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: pairwise_cuda|octree_gpu|octree_cpu)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::Integrator: {
        std::string canonical;
        if (!grav_modes::normalizeIntegrator(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: euler|rk4|leapfrog)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::OctreeCriterion: {
        std::string canonical;
        if (!grav_modes::normalizeOctreeOpeningCriterion(value, canonical)) {
            warnings << source << " invalid " << optionName << ": " << value
                     << " (allowed: com|bounds)\n";
            return true;
        }
        memberAt<std::string>(config, entry.offset) = canonical;
        return true;
    }
    case OptionKind::ClientParticleCap: {
        std::uint32_t parsed = memberAt<std::uint32_t>(config, entry.offset);
        if (!SimulationArgsParse::parseUint(value, parsed) || parsed < 2u) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        const std::uint32_t clamped = clampClientParticleCap(parsed);
        memberAt<std::uint32_t>(config, entry.offset) = clamped;
        if (clamped != parsed) {
            emitClamped(warnings, source, optionName, parsed, clamped);
        }
        markCustomPerformanceProfile(entry, config);
        return true;
    }
    case OptionKind::TimeoutTriple: {
        std::uint32_t parsed = config.clientRemoteCommandTimeoutMs;
        if (!SimulationArgsParse::parseUint(value, parsed) || parsed < 10u || parsed > 60000u) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        config.clientRemoteCommandTimeoutMs = parsed;
        config.clientRemoteStatusTimeoutMs = parsed;
        config.clientRemoteSnapshotTimeoutMs = parsed;
        return true;
    }
    }
    return false;
}

template <typename Matcher>
static bool applyMatchingEntry(Matcher matcher, const std::string& key, const std::string& value,
                               SimulationConfig& config, std::ostream& warnings,
                               std::string_view source)
{
    for (std::size_t index = 0; index < kSimulationOptionCount; ++index) {
        const SimulationOptionEntry& entry = kSimulationOptions[index];
        if (!matcher(entry, key)) {
            continue;
        }
        return applyEntry(entry, value, config, warnings, source, key);
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

} // namespace grav_config
