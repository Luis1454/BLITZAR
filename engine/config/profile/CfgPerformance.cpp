/*
 * @file engine/config/profile/CfgPerformance.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/profile/CfgPerformance.hpp"
#include "config/core/CfgConfig.hpp"
#include "protocol/PtcProtocol.hpp"
#include <algorithm>
#include <cctype>

namespace bltzr_config {
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
        config.clientParticleCap = bltzr_protocol::kSnapshotDefaultPoints;
        config.snapshotPublishPeriodMs = 50u;
        config.energyMeasureEverySteps = 30u;
        config.energySampleLimit = 256u;
        config.substepTargetDt = 0.0f;
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
    config.clientParticleCap = bltzr_protocol::kSnapshotMaxPoints;
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

bool normalizeTreePmModel(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(raw);
    if (lowered == "auto" || lowered == "legacy") {
        outCanonical = "auto";
        return true;
    }
    if (lowered == "local_grid" || lowered == "grid" || lowered == "direct") {
        outCanonical = "local_grid";
        return true;
    }
    if (lowered == "tree" || lowered == "octree") {
        outCanonical = "tree";
        return true;
    }
    if (lowered == "exact_tree" || lowered == "exact-tree" || lowered == "tree_exact" ||
        lowered == "quality_tree") {
        outCanonical = "exact_tree";
        return true;
    }
    if (lowered == "hybrid") {
        outCanonical = "hybrid";
        return true;
    }
    if (lowered == "pm_only" || lowered == "pm-only" || lowered == "pm") {
        outCanonical = "pm_only";
        return true;
    }
    return false;
}

bool normalizeTreePmLayout(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(raw);
    if (lowered == "auto") {
        outCanonical = "auto";
        return true;
    }
    if (lowered == "linear") {
        outCanonical = "linear";
        return true;
    }
    if (lowered == "gather_linear" || lowered == "gather-linear") {
        outCanonical = "gather_linear";
        return true;
    }
    if (lowered == "gather_morton" || lowered == "gather-morton" || lowered == "morton") {
        outCanonical = "gather_morton";
        return true;
    }
    return false;
}

bool normalizeTreePmPrecision(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(raw);
    if (lowered == "fp32" || lowered == "float" || lowered == "single") {
        outCanonical = "fp32";
        return true;
    }
    if (lowered == "fp64" || lowered == "double") {
        outCanonical = "fp64";
        return true;
    }
    return false;
}

bool normalizeTreePmAssignment(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(raw);
    if (lowered == "cic" || lowered == "cloud_in_cell" || lowered == "cloud-in-cell") {
        outCanonical = "cic";
        return true;
    }
    if (lowered == "tsc" || lowered == "triangular_shaped_cloud" ||
        lowered == "triangular-shaped-cloud") {
        outCanonical = "tsc";
        return true;
    }
    if (lowered == "pcs" || lowered == "particle_cubic_spline" ||
        lowered == "particle-cubic-spline") {
        outCanonical = "pcs";
        return true;
    }
    return false;
}

bool normalizeTreePmPreset(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(raw);
    if (lowered == "custom") {
        outCanonical = "custom";
        return true;
    }
    if (lowered == "pm_only" || lowered == "pm-only") {
        outCanonical = "pm_only";
        return true;
    }
    if (lowered == "local_grid_fast" || lowered == "local-grid-fast") {
        outCanonical = "local_grid_fast";
        return true;
    }
    if (lowered == "hybrid_balanced" || lowered == "hybrid-balanced") {
        outCanonical = "hybrid_balanced";
        return true;
    }
    if (lowered == "hybrid_quality" || lowered == "hybrid-quality") {
        outCanonical = "hybrid_quality";
        return true;
    }
    if (lowered == "tree_quality" || lowered == "tree-quality") {
        outCanonical = "tree_quality";
        return true;
    }
    return false;
}

void applyTreePmPreset(SimulationConfig& config)
{
    std::string canonical;
    if (!normalizeTreePmPreset(config.treePmPreset, canonical)) {
        config.treePmPreset = "custom";
        return;
    }
    config.treePmPreset = canonical;
    if (canonical == "custom") {
        return;
    }
    config.treePmEnabled = true;
    if (canonical == "pm_only") {
        config.treePmModel = "pm_only";
        config.treePmLocalGrid = true;
        config.treePmMaxLocalNeighbors = 0u;
        config.treePmGridSize = 64u;
        config.treePmJacobiIterations = 8u;
        return;
    }
    if (canonical == "local_grid_fast") {
        config.treePmModel = "local_grid";
        config.treePmLocalGrid = true;
        config.treePmMaxLocalNeighbors = 32u;
        config.treePmGridSize = 48u;
        config.treePmJacobiIterations = 8u;
        return;
    }
    if (canonical == "hybrid_balanced") {
        config.treePmModel = "hybrid";
        config.treePmLocalGrid = true;
        config.treePmMaxLocalNeighbors = 64u;
        config.treePmDenseCellThreshold = 64u;
        config.treePmGridSize = 64u;
        config.treePmJacobiIterations = 12u;
        return;
    }
    if (canonical == "hybrid_quality") {
        config.treePmModel = "hybrid";
        config.treePmLocalGrid = true;
        config.treePmMaxLocalNeighbors = 128u;
        config.treePmDenseCellThreshold = 32u;
        config.treePmGridSize = 96u;
        config.treePmJacobiIterations = 24u;
        return;
    }
    config.treePmModel = "exact_tree";
    config.treePmLocalGrid = false;
    config.treePmMaxLocalNeighbors = 0u;
    config.treePmGridSize = 96u;
    config.treePmJacobiIterations = 24u;
}
} // namespace bltzr_config
