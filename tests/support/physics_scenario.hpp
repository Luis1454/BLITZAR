/*
 * @file tests/support/physics_scenario.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef BLITZAR_TESTS_SUPPORT_PHYSICS_SCENARIO_HPP_
#define BLITZAR_TESTS_SUPPORT_PHYSICS_SCENARIO_HPP_
#include "server/SimulationServer.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace testsupport {
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

struct ScenarioResult {
    std::vector<RenderParticle> initial;
    std::vector<RenderParticle> final;
    SimulationStats stats{};
    float maxAbsEnergyDriftPct = 0.0f;
    float maxParticleDeltaFromInitial = 0.0f;
};

float distance(const RenderParticle& a, const RenderParticle& b);
std::array<float, 3> centerOfMassAll(const std::vector<RenderParticle>& snapshot);
float averageRadius(const std::vector<RenderParticle>& snapshot);
bool runScenario(const ScenarioConfig& cfg, ScenarioResult& out, std::string& error);
std::string getTwoBodyInputPath();
bool prepareTwoBodyScenario(ScenarioConfig& cfg, std::string& error);
bool prepareGeneratedCalibrationScenario(const std::string& mode, ScenarioConfig& cfg,
                                         std::string& error);
} // namespace testsupport
#endif // BLITZAR_TESTS_SUPPORT_PHYSICS_SCENARIO_HPP_
