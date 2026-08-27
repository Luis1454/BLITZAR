#ifndef BLITZAR_SIMULATION_INPUT_SIM_CONFIG_STATE_HPP
#define BLITZAR_SIMULATION_INPUT_SIM_CONFIG_STATE_HPP

#include "core/CoreTypes.hpp"
#include "simulation/input/SimConfigRun.hpp"

#include <vector>

namespace blitzar_sim {

struct SimConfigState final {
    std::vector<double> position_x;
    std::vector<double> position_y;
    std::vector<double> position_z;
    std::vector<double> velocity_x;
    std::vector<double> velocity_y;
    std::vector<double> velocity_z;
    std::vector<double> mass;

    [[nodiscard]] blitzar_core::ParticleStateView Input() const noexcept;
    [[nodiscard]] blitzar_core::ParticleOutputView Output() noexcept;
};

[[nodiscard]] blitzar_status BuildState(
    const SimConfigRun& config, SimConfigState& destination) noexcept;

} // namespace blitzar_sim

#endif
