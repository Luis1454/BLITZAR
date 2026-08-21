#ifndef BLITZAR_INTEGRATION_LEAPFROG_WORKSPACE_HPP
#define BLITZAR_INTEGRATION_LEAPFROG_WORKSPACE_HPP

#include "core/Types.hpp"
#include "particles/ParticleArray.hpp"

#include <blitzar/blitzar.h>

#include <cstddef>

namespace blitzar_integration {

class LeapfrogWorkspace final {
public:
    explicit LeapfrogWorkspace(std::size_t count);
    ~LeapfrogWorkspace() = default;

    LeapfrogWorkspace(const LeapfrogWorkspace&) = delete;
    LeapfrogWorkspace& operator=(const LeapfrogWorkspace&) = delete;

    LeapfrogWorkspace(LeapfrogWorkspace&&) noexcept = default;
    LeapfrogWorkspace& operator=(LeapfrogWorkspace&&) noexcept = default;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] blitzar_status Capture(
        blitzar_core::MutableParticleView state) noexcept;
    [[nodiscard]] blitzar_status Restore(
        blitzar_core::MutableParticleView state) noexcept;

private:
    std::size_t count_;
    blitzar_particles::ParticleArray position_x_;
    blitzar_particles::ParticleArray position_y_;
    blitzar_particles::ParticleArray position_z_;
    blitzar_particles::ParticleArray velocity_x_;
    blitzar_particles::ParticleArray velocity_y_;
    blitzar_particles::ParticleArray velocity_z_;
};

}  // namespace blitzar_integration

#endif
