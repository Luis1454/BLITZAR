// File: tests/support/physics_scenario.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_PHYSICS_SCENARIO_HPP_
#define GRAVITY_TESTS_SUPPORT_PHYSICS_SCENARIO_HPP_
#include "server/SimulationServer.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace testsupport {
/// Description: Defines the ScenarioConfig data or behavior contract.
struct ScenarioConfig {
    std::string inputPath;
    std::string inputFormat = "xyz";
    std::string solver = "pairwise_cuda";
    std::string integrator = "rk4";
    std::string performanceProfile = "interactive";
    float dt = 0.002f;
    float octreeTheta = 1.2f;
    float octreeSoftening = 2.5f;
    std::uint32_t steps = 100;
    std::uint32_t particleCount = 2u;
    std::uint32_t energyMeasureEverySteps = 1u;
    std::uint32_t energySampleLimit = 0u;
    int snapshotTimeoutMs = 3000;
    int stepTimeoutMs = 3000;
    bool sphEnabled = false;
    InitialStateConfig initState{};
};

/// Description: Defines the ScenarioResult data or behavior contract.
struct ScenarioResult {
    std::vector<RenderParticle> initial;
    std::vector<RenderParticle> final;
    SimulationStats stats{};
    float maxAbsEnergyDriftPct = 0.0f;
    float maxParticleDeltaFromInitial = 0.0f;
};

/// Description: Executes the distance operation.
float distance(const RenderParticle& a, const RenderParticle& b);
/// Description: Executes the centerOfMassAll operation.
std::array<float, 3> centerOfMassAll(const std::vector<RenderParticle>& snapshot);
/// Description: Executes the averageRadius operation.
float averageRadius(const std::vector<RenderParticle>& snapshot);
/// Description: Executes the runScenario operation.
bool runScenario(const ScenarioConfig& cfg, ScenarioResult& out, std::string& error);
/// Description: Executes the getTwoBodyInputPath operation.
std::string getTwoBodyInputPath();
/// Description: Executes the prepareTwoBodyScenario operation.
bool prepareTwoBodyScenario(ScenarioConfig& cfg, std::string& error);
/// Description: Describes the prepare generated calibration scenario operation contract.
bool prepareGeneratedCalibrationScenario(const std::string& mode, ScenarioConfig& cfg,
                                         std::string& error);
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_PHYSICS_SCENARIO_HPP_
