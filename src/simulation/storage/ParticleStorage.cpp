#include "simulation/storage/ParticleStorage.hpp"

namespace blitzar_sim {

ParticleStorage::ParticleStorage(std::size_t particle_count)
    : arena_(particle_count), particles_(arena_), accelerations_(arena_), checkpoint_(arena_)
{
}

blitzar_particles::ParticleArena& ParticleStorage::Arena() noexcept
{
    return arena_;
}

const blitzar_particles::ParticleArena& ParticleStorage::Arena() const noexcept
{
    return arena_;
}

blitzar_particles::ParticleBuffer& ParticleStorage::Particles() noexcept
{
    return particles_;
}

const blitzar_particles::ParticleBuffer& ParticleStorage::Particles() const noexcept
{
    return particles_;
}

blitzar_particles::AccelerationBuffer& ParticleStorage::Accelerations() noexcept
{
    return accelerations_;
}

const blitzar_particles::AccelerationBuffer& ParticleStorage::Accelerations() const noexcept
{
    return accelerations_;
}

blitzar_integration::KdkCheckpoint& ParticleStorage::Checkpoint() noexcept
{
    return checkpoint_;
}

const blitzar_integration::KdkCheckpoint& ParticleStorage::Checkpoint() const noexcept
{
    return checkpoint_;
}

} // namespace blitzar_sim
