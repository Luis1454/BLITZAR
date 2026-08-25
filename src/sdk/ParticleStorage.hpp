#ifndef BLITZAR_SDK_PARTICLE_STORAGE_HPP
#define BLITZAR_SDK_PARTICLE_STORAGE_HPP

#include "integration/KdkCheckpoint.hpp"
#include "particles/AccelerationBuffer.hpp"
#include "particles/ParticleBuffer.hpp"

#include <cstddef>

namespace blitzar_sdk {

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

} // namespace blitzar_sdk

#endif
