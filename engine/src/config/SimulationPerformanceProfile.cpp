/*
 * @file engine/src/config/SimulationPerformanceProfile.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"
#include <algorithm>
#include <cctype>

namespace grav_config {
static std::string toLowerProfile(std::string_view raw)
{
    std::string lowered(raw);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered;
}

bool normalizePerformanceProfile(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(raw);
    if (lowered == kPerformanceProfileInteractive) {
        outCanonical = std::string(kPerformanceProfileInteractive);
        return true;
    }
    if (lowered == kPerformanceProfileBalanced) {
        outCanonical = std::string(kPerformanceProfileBalanced);
        return true;
    }
    if (lowered == kPerformanceProfileQuality) {
        outCanonical = std::string(kPerformanceProfileQuality);
        return true;
    }
    if (lowered == kPerformanceProfileCustom) {
        outCanonical = std::string(kPerformanceProfileCustom);
        return true;
    }
    return false;
}

void applyPerformanceProfile(SimulationConfig& config)
{
    std::string canonical;
    if (!normalizePerformanceProfile(config.performanceProfile, canonical)) {
        canonical = std::string(kPerformanceProfileCustom);
    }
    config.performanceProfile = canonical;
    if (canonical == kPerformanceProfileCustom) {
        return;
    }
    if (canonical == kPerformanceProfileInteractive) {
        config.clientParticleCap = grav_protocol::kSnapshotDefaultPoints;
        config.snapshotPublishPeriodMs = 50u;
        config.energyMeasureEverySteps = 30u;
        config.energySampleLimit = 256u;
        config.substepTargetDt = 0.01f;
        config.maxSubsteps = 4u;
        return;
    }
    if (canonical == kPerformanceProfileBalanced) {
        config.clientParticleCap = 8192u;
        config.snapshotPublishPeriodMs = 33u;
        config.energyMeasureEverySteps = 20u;
        config.energySampleLimit = 1024u;
        config.substepTargetDt = 0.005f;
        config.maxSubsteps = 8u;
        return;
    }
    config.clientParticleCap = grav_protocol::kSnapshotMaxPoints;
    config.snapshotPublishPeriodMs = 16u;
    config.energyMeasureEverySteps = 10u;
    config.energySampleLimit = 5000u;
    config.substepTargetDt = 0.0f;
    config.maxSubsteps = 32u;
}

bool isPerformanceManagedField(std::string_view key)
{
    return key == "substep_target_dt" || key == "max_substeps" ||
           key == "snapshot_publish_period_ms" || key == "client_particle_cap" ||
           key == "energy_measure_every_steps" || key == "energy_sample_limit";
}
} // namespace grav_config
