/*
 * @file tests/support/physics_test_utils.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef GRAVITY_TESTS_SUPPORT_PHYSICS_TEST_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_PHYSICS_TEST_UTILS_HPP_
#include "tests/support/physics_scenario.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace testsupport {
void setScenarioTiming(ScenarioConfig& cfg, int snapshotTimeoutMs, int stepTimeoutMs);
void setScenarioEnergySampling(ScenarioConfig& cfg, std::uint32_t measureEverySteps,
                               std::uint32_t sampleLimit);
bool buildTwoBodyFileScenario(ScenarioConfig& outConfig, std::uint32_t steps, float dt,
                              const std::string& solver, const std::string& integrator,
                              std::string& error);
ScenarioConfig buildDiskOrbitScenario(std::uint32_t particleCount, float dt, std::uint32_t steps,
                                      std::uint32_t seed, const std::string& solver,
                                      const std::string& integrator);
ScenarioConfig buildRandomCloudScenario(std::uint32_t particleCount, float dt, std::uint32_t steps,
                                        std::uint32_t seed, const std::string& solver,
                                        const std::string& integrator);
bool waitForStepCount(const SimulationServer& server, std::uint64_t targetStep, int timeoutMs);
bool waitForConsumedSnapshot(SimulationServer& server, std::vector<RenderParticle>& outSnapshot,
                             int timeoutMs);
bool waitForPublishedSnapshot(const SimulationServer& server,
                              std::vector<RenderParticle>& outSnapshot, int timeoutMs);
bool haveExactReplayMatch(const ScenarioResult& lhs, const ScenarioResult& rhs, std::string& error);
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_PHYSICS_TEST_UTILS_HPP_
