#include "simulation/state/SimParticleState.hpp"

namespace blitzar_sim {

SimParticleState::SimParticleState(std::size_t particle_count)
    : arena_(particle_count), particles_(arena_), accelerations_(arena_), checkpoint_(arena_)
{
}

blitzar_particles::ParticleArena& SimParticleState::Arena() noexcept
{
    return arena_;
}

const blitzar_particles::ParticleArena& SimParticleState::Arena() const noexcept
{
    return arena_;
}

blitzar_particles::ParticleBuffer& SimParticleState::Particles() noexcept
{
    return particles_;
}

const blitzar_particles::ParticleBuffer& SimParticleState::Particles() const noexcept
{
    return particles_;
}

blitzar_particles::ParticleAccelerationBuffer& SimParticleState::Accelerations() noexcept
{
    return accelerations_;
}

const blitzar_particles::ParticleAccelerationBuffer&
SimParticleState::Accelerations() const noexcept
{
    return accelerations_;
}

blitzar_integration::KdkCheckpoint& SimParticleState::Checkpoint() noexcept
{
    return checkpoint_;
}

const blitzar_integration::KdkCheckpoint& SimParticleState::Checkpoint() const noexcept
{
    return checkpoint_;
}

} // namespace blitzar_sim
