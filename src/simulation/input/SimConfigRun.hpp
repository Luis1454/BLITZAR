#ifndef BLITZAR_SIMULATION_INPUT_SIM_CONFIG_RUN_HPP
#define BLITZAR_SIMULATION_INPUT_SIM_CONFIG_RUN_HPP

#include "simulation/input/SimConfigDiagnostics.hpp"
#include "simulation/input/SimConfigFile.hpp"
#include "simulation/input/SimConfigOutput.hpp"

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>

namespace blitzar_sim {

struct SimConfigBarnesHut final {
    double opening_angle{0.5};
    std::int64_t max_particles{};
    std::int64_t max_cells{};
    std::int64_t leaf_capacity{8};
    std::int64_t max_depth{32};
};

struct SimConfigRun final {
    static constexpr std::int64_t MaxParticleCount = 100000;
    static constexpr std::int64_t MaxSteps = 100000;

    std::int64_t particle_count{};
    double timestep{};
    blitzar_solver_kind solver{BLITZAR_SOLVER_DIRECT};
    blitzar_integrator_kind integrator{BLITZAR_INTEGRATOR_LEAPFROG_KDK};
    double gravitational_constant{};
    double softening{};
    double length_scale{};
    double mass_scale{};
    double time_scale{};
    SimConfigBarnesHut barnes_hut{};
    SimConfigOutput output{};
    SimConfigDiagnostics diagnostics{};
    std::uint64_t seed{};
    std::int64_t steps{1};
    bool deterministic{};
};

[[nodiscard]] blitzar_status BuildRunConfig(
    const SimConfigFile& source, SimConfigRun& destination) noexcept;

[[nodiscard]] blitzar_status BuildRunConfig(const SimConfigFile& source,
    const std::filesystem::path& config_directory, SimConfigRun& destination) noexcept;

} // namespace blitzar_sim

#endif
