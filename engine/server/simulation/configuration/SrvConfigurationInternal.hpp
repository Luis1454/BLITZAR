/*
 * @file engine/server/simulation/configuration/SrvConfigurationInternal.hpp
 * @brief Internal configuration and execution-mode contracts.
 */

#ifndef BLITZAR_ENGINE_SERVER_SIMULATION_CONFIGURATION_SRV_CONFIGURATION_INTERNAL_HPP_
#define BLITZAR_ENGINE_SERVER_SIMULATION_CONFIGURATION_SRV_CONFIGURATION_INTERNAL_HPP_

#include "server/simulation/runtime/SrvRuntimeBase.hpp"

std::string toLower(std::string value);
std::string trim(std::string value);
std::string normalizeSnapshotFormat(std::string format);
std::string executableSolverMode(std::string solver);
float clampSimulationDt(float dt);
ParticleSystem::SolverMode solverModeFromCanonicalName(std::string_view name);
ParticleSystem::IntegratorMode integratorModeFromCanonicalName(std::string_view name);
std::string_view solverLabel(ParticleSystem::SolverMode mode);

constexpr std::size_t kMaxImportedParticles = 2'000'000;
constexpr std::uint32_t kPairwiseRealtimeParticleLimit = 20'000u;

std::uint32_t resolvePublishedSnapshotCap(std::uint32_t drawCap);
std::string readEnvironment(std::string_view key);
bool isValidImportedParticleCount(std::size_t count);
bool isAutoSolverFallbackEnabled();
bool shouldForceCudaFailureOnceForTesting(std::string_view solver);
bool coerceConfigSolverIntegratorCompatibility(std::string& solver, std::string& integrator,
                                               std::string_view source);
float autoTargetSubstepDt(std::string_view solver, bool eulerIntegrator, bool sphEnabled,
                          std::size_t liveParticleCount);
OctreeOpeningCriterion openingCriterionFromCanonicalName(std::string_view name);
float clampThetaBound(float value);
float computeOctreeDistributionScore(const std::vector<Particle>& particles);
float profileThetaBias(std::string_view performanceProfile);
float particleThetaBias(std::size_t particleCount);
float resolveOctreeTheta(float configuredTheta, bool autoTune, float autoMin, float autoMax,
                         std::string_view performanceProfile,
                         const std::vector<Particle>& particles, float distributionScore);
void logEffectiveExecutionModes(
    std::string_view solver, std::string_view integrator, std::string_view performanceProfile,
    std::string_view openingCriterion, float theta, float effectiveTheta, bool thetaAutoTune,
    float thetaAutoMin, float thetaAutoMax, float octreeDistributionScore, float softening,
    float physicsMaxAcceleration, float physicsMinSoftening, float physicsMinDistance2,
    float physicsMinTheta, bool sphEnabled, float configuredSubstepTargetDt,
    std::uint32_t configuredMaxSubsteps, std::uint32_t snapshotPublishPeriodMs, float serverFps,
    float energyDriftPct);

#endif
