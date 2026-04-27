// File: tests/support/physics_test_utils.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_PHYSICS_TEST_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_PHYSICS_TEST_UTILS_HPP_
#include "tests/support/physics_scenario.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace testsupport {
/// Description: Executes the setScenarioTiming operation.
void setScenarioTiming(ScenarioConfig& cfg, int snapshotTimeoutMs, int stepTimeoutMs);
/// Description: Describes the set scenario energy sampling operation contract.
void setScenarioEnergySampling(ScenarioConfig& cfg, std::uint32_t measureEverySteps,
                               std::uint32_t sampleLimit);
/// Description: Describes the build two body file scenario operation contract.
bool buildTwoBodyFileScenario(ScenarioConfig& outConfig, std::uint32_t steps, float dt,
                              const std::string& solver, const std::string& integrator,
                              std::string& error);
/// Description: Describes the build disk orbit scenario operation contract.
ScenarioConfig buildDiskOrbitScenario(std::uint32_t particleCount, float dt, std::uint32_t steps,
                                      std::uint32_t seed, const std::string& solver,
                                      const std::string& integrator);
/// Description: Describes the build random cloud scenario operation contract.
ScenarioConfig buildRandomCloudScenario(std::uint32_t particleCount, float dt, std::uint32_t steps,
                                        std::uint32_t seed, const std::string& solver,
                                        const std::string& integrator);
/// Description: Executes the waitForStepCount operation.
bool waitForStepCount(const SimulationServer& server, std::uint64_t targetStep, int timeoutMs);
/// Description: Describes the wait for consumed snapshot operation contract.
bool waitForConsumedSnapshot(SimulationServer& server, std::vector<RenderParticle>& outSnapshot,
                             int timeoutMs);
/// Description: Describes the wait for published snapshot operation contract.
bool waitForPublishedSnapshot(const SimulationServer& server,
                              std::vector<RenderParticle>& outSnapshot, int timeoutMs);
/// Description: Executes the haveExactReplayMatch operation.
bool haveExactReplayMatch(const ScenarioResult& lhs, const ScenarioResult& rhs, std::string& error);
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_PHYSICS_TEST_UTILS_HPP_
