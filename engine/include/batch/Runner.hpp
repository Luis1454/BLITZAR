/*
 * @file engine/include/batch/Runner.hpp
 * @brief Direct, synchronous execution contract for reproducible batch simulations.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_BATCH_RUNNER_HPP_
#define BLITZAR_ENGINE_INCLUDE_BATCH_RUNNER_HPP_

#include "config/core/Config.hpp"
#include "server/SimulationInitConfig.hpp"

#include <cstdint>
#include <string>

namespace bltzr_batch {
struct RunRequest {
    SimulationConfig config;
    ResolvedInitialStatePlan initialState;
    std::uint32_t targetSteps = 0u;
    bool exportOnExit = false;
    std::string exportPath;
};

struct RunResult {
    std::uint32_t particleCount = 0u;
    std::uint32_t steps = 0u;
    float simulatedTime = 0.0f;
    float cosmologyScaleFactor = 1.0f;
    std::string executionBackend;
    std::int64_t initializationMilliseconds = 0;
    std::int64_t integrationMilliseconds = 0;
    std::int64_t exportMilliseconds = 0;
    std::int64_t elapsedMilliseconds = 0;
    bool faulted = false;
    std::string solver;
    std::string integrator;
    std::string exportPath;
    std::string error;
};

class Runner {
public:
    RunResult run(const RunRequest& request) const;
};
} // namespace bltzr_batch

#endif // BLITZAR_ENGINE_INCLUDE_BATCH_RUNNER_HPP_
