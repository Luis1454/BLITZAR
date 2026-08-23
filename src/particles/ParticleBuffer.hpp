#ifndef BLITZAR_PARTICLES_PARTICLE_BUFFER_HPP
#define BLITZAR_PARTICLES_PARTICLE_BUFFER_HPP

#include "ParticleArena.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace blitzar_particles {

class ParticleBuffer final {
public:
    explicit ParticleBuffer(std::size_t count);
    explicit ParticleBuffer(ParticleArena& arena);
    ~ParticleBuffer() = default;

    ParticleBuffer(const ParticleBuffer&) = delete;
    ParticleBuffer& operator=(const ParticleBuffer&) = delete;

    ParticleBuffer(ParticleBuffer&& other) noexcept;
    ParticleBuffer& operator=(ParticleBuffer&& other) noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] blitzar_status SetCount(std::size_t count) noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_core::ParticleStateView State() const noexcept;
    [[nodiscard]] blitzar_core::MutableParticleView MutableView() noexcept;

    [[nodiscard]] blitzar_status SetPosition(
        std::size_t index, blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] blitzar_status SetVelocity(
        std::size_t index, blitzar_core::Vector3 velocity) noexcept;
    [[nodiscard]] blitzar_status SetMass(std::size_t index, blitzar_core::Scalar mass) noexcept;

private:
    [[nodiscard]] bool HasArena() const noexcept;
    [[nodiscard]] ParticleArena& Arena() const noexcept;

    std::unique_ptr<ParticleArena> owned_arena_;
    std::optional<std::reference_wrapper<ParticleArena>> borrowed_arena_;
    std::size_t count_;
};

class AccelerationBuffer final {
public:
    explicit AccelerationBuffer(std::size_t count);
    explicit AccelerationBuffer(ParticleArena& arena);
    ~AccelerationBuffer() = default;

    AccelerationBuffer(const AccelerationBuffer&) = delete;
    AccelerationBuffer& operator=(const AccelerationBuffer&) = delete;

    AccelerationBuffer(AccelerationBuffer&& other) noexcept;
    AccelerationBuffer& operator=(AccelerationBuffer&& other) noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] blitzar_status SetCount(std::size_t count) noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_core::ForceView View() noexcept;

private:
    [[nodiscard]] bool HasArena() const noexcept;
    [[nodiscard]] ParticleArena& Arena() const noexcept;

    std::unique_ptr<ParticleArena> owned_arena_;
    std::optional<std::reference_wrapper<ParticleArena>> borrowed_arena_;
    std::size_t count_;
};

} // namespace blitzar_particles

#endif
