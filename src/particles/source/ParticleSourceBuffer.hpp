#ifndef BLITZAR_PARTICLES_SOURCE_PARTICLE_SOURCE_BUFFER_HPP
#define BLITZAR_PARTICLES_SOURCE_PARTICLE_SOURCE_BUFFER_HPP

#include "core/CoreTypes.hpp"

#include <array>
#include <blitzar/blitzar.h>
#include <cstddef>
#include <span>
#include <vector>

namespace blitzar_particles {

class ParticleSourceBuffer final {
public:
    explicit ParticleSourceBuffer(std::size_t capacity = 0);
    ~ParticleSourceBuffer() = default;

    ParticleSourceBuffer(const ParticleSourceBuffer&) = delete;
    ParticleSourceBuffer& operator=(const ParticleSourceBuffer&) = delete;
    ParticleSourceBuffer(ParticleSourceBuffer&&) = delete;
    ParticleSourceBuffer& operator=(ParticleSourceBuffer&&) = delete;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t Capacity() const noexcept;
    [[nodiscard]] blitzar_status Reserve(std::size_t capacity) noexcept;
    [[nodiscard]] blitzar_status SetCount(std::size_t count) noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_core::ParticleStateView State() const noexcept;

    [[nodiscard]] std::span<blitzar_core::Scalar> PositionX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> PositionY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> PositionZ() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> VelocityX() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> VelocityY() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> VelocityZ() noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> Mass() noexcept;

private:
    enum class Field : std::size_t {
        PositionX,
        PositionY,
        PositionZ,
        VelocityX,
        VelocityY,
        VelocityZ,
        Mass,
    };

    static constexpr std::size_t Alignment = 64;
    static constexpr std::size_t FieldCount = 7;
    static constexpr std::size_t ScalarsPerAlignment = Alignment / sizeof(blitzar_core::Scalar);

    [[nodiscard]] static std::size_t AlignedCount(std::size_t count);
    [[nodiscard]] std::span<blitzar_core::Scalar> Mutable(Field field) noexcept;

    std::size_t count_{0};
    std::size_t capacity_{0};
    std::size_t stride_{0};
    std::vector<blitzar_core::Scalar> storage_;
    std::array<std::span<blitzar_core::Scalar>, FieldCount> fields_{};
};

} // namespace blitzar_particles

#endif
