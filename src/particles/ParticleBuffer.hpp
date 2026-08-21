#ifndef BLITZAR_PARTICLES_PARTICLE_BUFFER_HPP
#define BLITZAR_PARTICLES_PARTICLE_BUFFER_HPP

#include "ParticleArray.hpp"

#include <cstddef>

namespace blitzar_particles {

class ParticleBuffer final {
public:
    explicit ParticleBuffer(std::size_t count);
    ~ParticleBuffer() = default;

    ParticleBuffer(const ParticleBuffer&) = delete;
    ParticleBuffer& operator=(const ParticleBuffer&) = delete;

    ParticleBuffer(ParticleBuffer&&) noexcept = default;
    ParticleBuffer& operator=(ParticleBuffer&&) noexcept = default;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_core::ParticleStateView State() const noexcept;
    [[nodiscard]] blitzar_core::MutableParticleView MutableView() noexcept;

    void SetPosition(std::size_t index, blitzar_core::Vector3 position) noexcept;
    void SetVelocity(std::size_t index, blitzar_core::Vector3 velocity) noexcept;
    void SetMass(std::size_t index, blitzar_core::Scalar mass) noexcept;

private:
    std::size_t count_;
    ParticleArray position_x_;
    ParticleArray position_y_;
    ParticleArray position_z_;
    ParticleArray velocity_x_;
    ParticleArray velocity_y_;
    ParticleArray velocity_z_;
    ParticleArray mass_;
};

class AccelerationBuffer final {
public:
    explicit AccelerationBuffer(std::size_t count);
    ~AccelerationBuffer() = default;

    AccelerationBuffer(const AccelerationBuffer&) = delete;
    AccelerationBuffer& operator=(const AccelerationBuffer&) = delete;

    AccelerationBuffer(AccelerationBuffer&&) noexcept = default;
    AccelerationBuffer& operator=(AccelerationBuffer&&) noexcept = default;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] blitzar_core::ForceView View() noexcept;

private:
    std::size_t count_;
    ParticleArray x_;
    ParticleArray y_;
    ParticleArray z_;
};

}  // namespace blitzar_particles

#endif
