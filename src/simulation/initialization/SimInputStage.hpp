#ifndef BLITZAR_SIMULATION_INITIALIZATION_SIM_INPUT_STAGE_HPP
#define BLITZAR_SIMULATION_INITIALIZATION_SIM_INPUT_STAGE_HPP

#include "core/CoreTypes.hpp"

#include <blitzar/blitzar.h>
#include <vector>

namespace blitzar_sim {

struct SimInputStage final {
    std::vector<blitzar_core::Scalar> position_x;
    std::vector<blitzar_core::Scalar> position_y;
    std::vector<blitzar_core::Scalar> position_z;
    std::vector<blitzar_core::Scalar> velocity_x;
    std::vector<blitzar_core::Scalar> velocity_y;
    std::vector<blitzar_core::Scalar> velocity_z;
    std::vector<blitzar_core::Scalar> mass;

    [[nodiscard]] blitzar_core::ParticleStateView State() const noexcept;
};

[[nodiscard]] blitzar_status StageParticleInput(
    blitzar_core::ParticleStateView input, SimInputStage& stage) noexcept;

} // namespace blitzar_sim

#endif
