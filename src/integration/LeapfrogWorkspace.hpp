#ifndef BLITZAR_INTEGRATION_LEAPFROG_WORKSPACE_HPP
#define BLITZAR_INTEGRATION_LEAPFROG_WORKSPACE_HPP

#include "core/Types.hpp"
#include "particles/ParticleArena.hpp"

#include <blitzar/blitzar.h>

#include <cstddef>
#include <memory>

namespace blitzar_integration {

class LeapfrogWorkspace final {
public:
    explicit LeapfrogWorkspace(std::size_t count);
    explicit LeapfrogWorkspace(
        std::shared_ptr<blitzar_particles::ParticleArena> arena);
    ~LeapfrogWorkspace() = default;

    LeapfrogWorkspace(const LeapfrogWorkspace&) = delete;
    LeapfrogWorkspace& operator=(const LeapfrogWorkspace&) = delete;

    LeapfrogWorkspace(LeapfrogWorkspace&& other) noexcept;
    LeapfrogWorkspace& operator=(LeapfrogWorkspace&& other) noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_status Capture(
        blitzar_core::MutableParticleView state) noexcept;
    [[nodiscard]] blitzar_status Restore(
        blitzar_core::MutableParticleView state) noexcept;

private:
    std::size_t count_;
    std::shared_ptr<blitzar_particles::ParticleArena> arena_;
};

}  // namespace blitzar_integration

#endif
