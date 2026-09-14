#ifndef BLITZAR_PHYSICS_SPH_SPH_DENSITY_HPP
#define BLITZAR_PHYSICS_SPH_SPH_DENSITY_HPP

#include "core/CoreTypes.hpp"
#include "physics/neighbors/NeighborIndex.hpp"

#include <blitzar/c/blitzar.h>
#include <cstddef>
#include <span>

namespace blitzar_physics {

struct SphDensitySettings final {
    bool normalize_boundary{false};
};

struct SphDensityRequest final {
    const NeighborIndex& neighbor_index;
    std::span<const blitzar_core::Scalar> x{};
    std::span<const blitzar_core::Scalar> y{};
    std::span<const blitzar_core::Scalar> z{};
    std::span<const blitzar_core::Scalar> mass{};
    std::span<const blitzar_core::Scalar> smoothing_length{};
};

class SphDensity final {
public:
    SphDensity() noexcept = default;
    explicit SphDensity(SphDensitySettings settings) noexcept;

    [[nodiscard]] static blitzar_core::Scalar Weight(
        blitzar_core::Scalar distance, blitzar_core::Scalar smoothing_length) noexcept;

    [[nodiscard]] blitzar_status Evaluate(
        const SphDensityRequest& request, std::span<blitzar_core::Scalar> density) noexcept;

private:
    SphDensitySettings settings_{};
};

} // namespace blitzar_physics

#endif
