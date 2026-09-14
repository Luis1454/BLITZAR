#include "physics/sph/SphDensity.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

namespace blitzar_physics {

namespace {

constexpr blitzar_core::Scalar kPi = blitzar_core::Scalar{3.141592653589793238462643383279502884};

[[nodiscard]] bool IsFinite(blitzar_core::Scalar value) noexcept
{
    return value == value && value <= std::numeric_limits<blitzar_core::Scalar>::max() &&
           value >= -std::numeric_limits<blitzar_core::Scalar>::max();
}

[[nodiscard]] blitzar_core::Scalar DistanceBetween(
    const SphDensityRequest& request, std::size_t left, std::size_t right) noexcept
{
    const blitzar_core::Scalar dx = request.x[left] - request.x[right];
    const blitzar_core::Scalar dy = request.y[left] - request.y[right];
    const blitzar_core::Scalar dz = request.z[left] - request.z[right];

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

SphDensity::SphDensity(SphDensitySettings settings) noexcept : settings_(settings) {}

blitzar_core::Scalar SphDensity::Weight(
    blitzar_core::Scalar distance, blitzar_core::Scalar smoothing_length) noexcept
{
    if (distance >= blitzar_core::Scalar{2} * smoothing_length ||

        smoothing_length <= blitzar_core::Scalar{0}) {
        return blitzar_core::Scalar{0};
    }

    const blitzar_core::Scalar q = distance / smoothing_length;

    const blitzar_core::Scalar scale =
        (blitzar_core::Scalar{1} / (kPi * smoothing_length * smoothing_length * smoothing_length));

    if (q >= blitzar_core::Scalar{1}) {
        const blitzar_core::Scalar tail = blitzar_core::Scalar{2} - q;

        return (blitzar_core::Scalar{0.25} * scale) * tail * tail * tail;
    }

    return scale * (blitzar_core::Scalar{1} - blitzar_core::Scalar{1.5} * q * q +
                       blitzar_core::Scalar{0.75} * q * q * q);
}

blitzar_status SphDensity::Evaluate(
    const SphDensityRequest& request, std::span<blitzar_core::Scalar> density) noexcept
{
    const std::size_t count = request.x.size();

    if (request.y.size() != count || request.z.size() != count || request.mass.size() != count ||
        request.smoothing_length.size() != count || density.size() != count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (count == 0) {
        return BLITZAR_STATUS_OK;
    }

    if (!request.neighbor_index.IsBuilt()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < count; ++index) {
        if (!IsFinite(request.mass[index]) || request.mass[index] < blitzar_core::Scalar{0}) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        if (request.smoothing_length[index] <= blitzar_core::Scalar{0} ||

            !IsFinite(request.smoothing_length[index])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    for (std::size_t index = 0; index < count; ++index) {
        if (!request.neighbor_index.IsBuilt() ||
            request.neighbor_index.Query(index).indices.size() == 0) {
            return BLITZAR_STATUS_SINGULARITY;
        }

        blitzar_core::Scalar neighborhood_mass = request.mass[index];

        for (const std::size_t source : request.neighbor_index.Query(index).indices) {
            neighborhood_mass += request.mass[source];
        }

        if (neighborhood_mass <= blitzar_core::Scalar{0}) {
            return BLITZAR_STATUS_SINGULARITY;
        }
    }

    for (std::size_t index = 0; index < count; ++index) {
        const blitzar_core::Scalar smooth = request.smoothing_length[index];
        blitzar_core::Scalar weight_sum = SphDensity::Weight(blitzar_core::Scalar{0}, smooth);
        blitzar_core::Scalar density_sum = request.mass[index] * weight_sum;

        for (const std::size_t source : request.neighbor_index.Query(index).indices) {
            const blitzar_core::Scalar distance = DistanceBetween(request, index, source);

            const blitzar_core::Scalar weight = SphDensity::Weight(distance, smooth);

            density_sum += request.mass[source] * weight;
            weight_sum += weight;
        }

        blitzar_core::Scalar density_value = density_sum;

        if (settings_.normalize_boundary) {
            if (weight_sum <= blitzar_core::Scalar{0}) {
                return BLITZAR_STATUS_SINGULARITY;
            }

            density_value = density_value / weight_sum;
        }

        if (!IsFinite(density_value) || density_value <= blitzar_core::Scalar{0}) {
            return BLITZAR_STATUS_SINGULARITY;
        }

        density[index] = density_value;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_physics
