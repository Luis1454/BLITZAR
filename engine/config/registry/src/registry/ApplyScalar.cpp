/*
 * @file engine/config/registry/src/registry/ApplyScalar.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Scalar and grouped value conversion for registry entries.
 */

#include "Internal.hpp"

#include "core/Config.hpp"
#include "profile/Performance.hpp"
#include "protocol/Protocol.hpp"

#include <ostream>

namespace bltzr_config {

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
             << bltzr_protocol::kSnapshotMaxPoints << "]: " << requested << " -> " << clamped
             << "\n";
}

static std::uint32_t clampClientParticleCap(std::uint32_t requested)
{
    if (requested > bltzr_protocol::kSnapshotMaxPoints) {
        return bltzr_protocol::kSnapshotMaxPoints;
    }
    return requested < 2u ? 2u : requested;
}

static void markCustomPerformanceProfile(const SimulationOptionEntry& entry,
                                         SimulationConfig& config)
{
    if (isPerformanceManagedField(entry.iniName)) {
        config.performanceProfile = std::string(kPerformanceProfileCustom);
    }
}

bool applyScalarEntry(const SimulationOptionEntry& entry, const std::string& value,
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
        if (!SimulationArgsParse::parseUint(value, parsed) || parsed < kRuntimeRemoteTimeoutMinMs ||
            parsed > kRuntimeRemoteTimeoutMaxMs) {
            emitInvalid(warnings, source, optionName, value);
            return true;
        }
        config.clientRemoteCommandTimeoutMs = parsed;
        config.clientRemoteStatusTimeoutMs = parsed;
        config.clientRemoteSnapshotTimeoutMs = parsed;
        return true;
    }
    default:
        return false;
    }
}

} // namespace bltzr_config
