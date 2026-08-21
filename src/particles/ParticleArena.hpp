#ifndef BLITZAR_PARTICLES_PARTICLE_ARENA_HPP
#define BLITZAR_PARTICLES_PARTICLE_ARENA_HPP

#include "core/Types.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace blitzar_particles {

class ParticleArena final {
public:
    explicit ParticleArena(std::size_t count);
    ~ParticleArena() = default;

    ParticleArena(const ParticleArena&) = delete;
    ParticleArena& operator=(const ParticleArena&) = delete;
    ParticleArena(ParticleArena&&) = delete;
    ParticleArena& operator=(ParticleArena&&) = delete;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] bool IsValid() const noexcept;

    [[nodiscard]] std::span<blitzar_core::Scalar> PositionX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> PositionY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> PositionZ() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> VelocityX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> VelocityY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> VelocityZ() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> Mass() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> AccelerationX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> AccelerationY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> AccelerationZ() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> WorkspacePositionX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> WorkspacePositionY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> WorkspacePositionZ() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> WorkspaceVelocityX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> WorkspaceVelocityY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> WorkspaceVelocityZ() noexcept;

private:
    enum class Field : std::size_t {
        PositionX,
        PositionY,
        PositionZ,
        VelocityX,
        VelocityY,
        VelocityZ,
        Mass,
        AccelerationX,
        AccelerationY,
        AccelerationZ,
        WorkspacePositionX,
        WorkspacePositionY,
        WorkspacePositionZ,
        WorkspaceVelocityX,
        WorkspaceVelocityY,
        WorkspaceVelocityZ,
    };

    static constexpr std::size_t Alignment = 64;
    static constexpr std::size_t FieldCount = 16;
    static constexpr std::size_t ScalarsPerAlignment =
        Alignment / sizeof(blitzar_core::Scalar);

    [[nodiscard]] static std::size_t AlignedCount(std::size_t count);
    [[nodiscard]] std::span<blitzar_core::Scalar> Mutable(Field field) noexcept;

    std::size_t count_;
    std::size_t stride_;
    std::vector<blitzar_core::Scalar> storage_;
    std::array<std::span<blitzar_core::Scalar>, FieldCount> fields_{};
};

}  // namespace blitzar_particles

#endif
