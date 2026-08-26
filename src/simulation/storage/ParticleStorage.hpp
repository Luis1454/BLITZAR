#ifndef BLITZAR_SIMULATION_STORAGE_PARTICLE_STORAGE_HPP
#define BLITZAR_SIMULATION_STORAGE_PARTICLE_STORAGE_HPP

#include "integration/kdk/KdkCheckpoint.hpp"
#include "particles/buffers/AccelerationBuffer.hpp"
#include "particles/buffers/ParticleBuffer.hpp"

#include <cstddef>

namespace blitzar_sim {

class ParticleStorage final {
public:
    explicit ParticleStorage(std::size_t particle_count);

    ParticleStorage(const ParticleStorage&) = delete;
    ParticleStorage& operator=(const ParticleStorage&) = delete;
    ParticleStorage(ParticleStorage&&) = delete;
    ParticleStorage& operator=(ParticleStorage&&) = delete;

    [[nodiscard]] blitzar_particles::ParticleArena& Arena() noexcept;
    [[nodiscard]] const blitzar_particles::ParticleArena& Arena() const noexcept;
    [[nodiscard]] blitzar_particles::ParticleBuffer& Particles() noexcept;
    [[nodiscard]] const blitzar_particles::ParticleBuffer& Particles() const noexcept;
    [[nodiscard]] blitzar_particles::AccelerationBuffer& Accelerations() noexcept;
    [[nodiscard]] const blitzar_particles::AccelerationBuffer& Accelerations() const noexcept;
    [[nodiscard]] blitzar_integration::KdkCheckpoint& Checkpoint() noexcept;
    [[nodiscard]] const blitzar_integration::KdkCheckpoint& Checkpoint() const noexcept;

private:
    blitzar_particles::ParticleArena arena_;
    blitzar_particles::ParticleBuffer particles_;
    blitzar_particles::AccelerationBuffer accelerations_;
    blitzar_integration::KdkCheckpoint checkpoint_;
};

} // namespace blitzar_sim

#endif
