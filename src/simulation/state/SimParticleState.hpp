#ifndef BLITZAR_SIMULATION_STATE_SIM_PARTICLE_STATE_HPP
#define BLITZAR_SIMULATION_STATE_SIM_PARTICLE_STATE_HPP

#include "integration/kdk/KdkCheckpoint.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"

#include <cstddef>

namespace blitzar_sim {

class SimParticleState final {
public:
    explicit SimParticleState(std::size_t particle_count);

    SimParticleState(const SimParticleState&) = delete;
    SimParticleState& operator=(const SimParticleState&) = delete;
    SimParticleState(SimParticleState&&) = delete;
    SimParticleState& operator=(SimParticleState&&) = delete;

    [[nodiscard]] blitzar_particles::ParticleArena& Arena() noexcept;
    [[nodiscard]] const blitzar_particles::ParticleArena& Arena() const noexcept;
    [[nodiscard]] blitzar_particles::ParticleBuffer& Particles() noexcept;
    [[nodiscard]] const blitzar_particles::ParticleBuffer& Particles() const noexcept;
    [[nodiscard]] blitzar_particles::ParticleAccelerationBuffer& Accelerations() noexcept;
    [[nodiscard]] const blitzar_particles::ParticleAccelerationBuffer&
    Accelerations() const noexcept;
    [[nodiscard]] blitzar_integration::KdkCheckpoint& Checkpoint() noexcept;
    [[nodiscard]] const blitzar_integration::KdkCheckpoint& Checkpoint() const noexcept;

private:
    blitzar_particles::ParticleArena arena_;
    blitzar_particles::ParticleBuffer particles_;
    blitzar_particles::ParticleAccelerationBuffer accelerations_;
    blitzar_integration::KdkCheckpoint checkpoint_;
};

} // namespace blitzar_sim

#endif
