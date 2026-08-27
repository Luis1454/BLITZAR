#ifndef BLITZAR_PARTICLES_BUFFER_PARTICLE_ACCELERATION_BUFFER_HPP
#define BLITZAR_PARTICLES_BUFFER_PARTICLE_ACCELERATION_BUFFER_HPP

#include "particles/arena/ParticleArena.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace blitzar_particles {

class ParticleAccelerationBuffer final {
public:
    explicit ParticleAccelerationBuffer(std::size_t count);
    explicit ParticleAccelerationBuffer(ParticleArena& arena);
    ~ParticleAccelerationBuffer() = default;

    ParticleAccelerationBuffer(const ParticleAccelerationBuffer&) = delete;
    ParticleAccelerationBuffer& operator=(const ParticleAccelerationBuffer&) = delete;

    ParticleAccelerationBuffer(ParticleAccelerationBuffer&& other) noexcept;
    ParticleAccelerationBuffer& operator=(ParticleAccelerationBuffer&& other) noexcept;

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
