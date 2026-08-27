#include "simulation/initialization/SimConfigState.hpp"

#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_sim {

blitzar_core::ParticleStateView SimConfigState::Input() const noexcept
{
    return {position_x.size(), position_x, position_y, position_z, velocity_x, velocity_y,
        velocity_z, mass, position_x.size()};
}

blitzar_core::ParticleOutputView SimConfigState::Output() noexcept
{
    return {position_x.size(), position_x, position_y, position_z, velocity_x, velocity_y,
        velocity_z, mass};
}

blitzar_status BuildState(const SimConfigRun& config, SimConfigState& destination) noexcept
{
    if (config.particle_count <= 0 || config.particle_count > SimConfigRun::MaxParticleCount) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    SimConfigState candidate;

    try {
        const std::size_t count = static_cast<std::size_t>(config.particle_count);

        candidate.position_x.resize(count);
        candidate.position_y.resize(count);
        candidate.position_z.resize(count);
        candidate.velocity_x.resize(count);
        candidate.velocity_y.resize(count);
        candidate.velocity_z.resize(count);
        candidate.mass.resize(count);

        for (std::size_t index = 0; index < count; ++index) {
            const std::uint64_t mixed_seed =
                config.seed + static_cast<std::uint64_t>(index) * UINT64_C(0x9E3779B97F4A7C15);

            const double jitter = static_cast<double>(mixed_seed % UINT64_C(1000003)) /
                                  static_cast<double>(UINT64_C(1000003));

            candidate.position_x[index] = static_cast<double>(index) + jitter * 0.001;
            candidate.position_y[index] = static_cast<double>(index % 3U) * 0.5;
            candidate.position_z[index] = static_cast<double>(index % 5U) * 0.25;
            candidate.velocity_x[index] = 0.0;
            candidate.velocity_y[index] = 0.0;
            candidate.velocity_z[index] = 0.0;
            candidate.mass[index] = 1.0;
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    destination = std::move(candidate);

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
