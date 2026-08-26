#ifndef BLITZAR_PARTICLES_BUFFERS_ACCELERATION_BUFFER_HPP
#define BLITZAR_PARTICLES_BUFFERS_ACCELERATION_BUFFER_HPP

#include "particles/arena/ParticleArena.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace blitzar_particles {

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
