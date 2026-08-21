#ifndef BLITZAR_PARTICLES_PARTICLE_ARRAY_HPP
#define BLITZAR_PARTICLES_PARTICLE_ARRAY_HPP

#include "core/Types.hpp"

#include <cstddef>
#include <memory>

namespace blitzar_particles {

class ParticleArray final {
public:
    explicit ParticleArray(std::size_t count);
    ~ParticleArray() = default;

    ParticleArray(const ParticleArray&) = delete;
    ParticleArray& operator=(const ParticleArray&) = delete;

    ParticleArray(ParticleArray&&) noexcept = default;
    ParticleArray& operator=(ParticleArray&&) noexcept = default;

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] blitzar_core::Scalar* Data() noexcept;
    [[nodiscard]] const blitzar_core::Scalar* Data() const noexcept;
    void Fill(blitzar_core::Scalar value) noexcept;

private:
    struct Deleter final {
        void operator()(blitzar_core::Scalar* data) const noexcept;
    };

    std::size_t count_;
    std::unique_ptr<blitzar_core::Scalar[], Deleter> data_;
};

}  // namespace blitzar_particles

#endif
