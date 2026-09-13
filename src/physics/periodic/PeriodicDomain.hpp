#ifndef BLITZAR_PHYSICS_PERIODIC_PERIODIC_DOMAIN_HPP
#define BLITZAR_PHYSICS_PERIODIC_PERIODIC_DOMAIN_HPP

#include "core/CoreTypes.hpp"

#include <cstddef>

namespace blitzar_physics {

class PeriodicDomain final {
public:
    PeriodicDomain() noexcept = default;

    PeriodicDomain(bool enabled, blitzar_core::Vector3 origin, blitzar_core::Vector3 size) noexcept;

    [[nodiscard]] bool IsEnabled() const noexcept;
    [[nodiscard]] bool IsValid() const noexcept;

    [[nodiscard]] blitzar_core::Vector3 Wrap(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] blitzar_core::Vector3 Fold(blitzar_core::Vector3 displacement) const noexcept;
    [[nodiscard]] bool Contains(blitzar_core::Vector3 center, blitzar_core::Scalar half_extent,
        blitzar_core::Vector3 position) const noexcept;

    [[nodiscard]] static std::size_t ImageCount() noexcept;
    [[nodiscard]] blitzar_core::Vector3 ImageShift(std::size_t index) const noexcept;

private:
    [[nodiscard]] static blitzar_core::Scalar WrapScalar(
        blitzar_core::Scalar value, blitzar_core::Scalar period) noexcept;
    [[nodiscard]] static blitzar_core::Scalar FoldScalar(
        blitzar_core::Scalar value, blitzar_core::Scalar period) noexcept;
    [[nodiscard]] static blitzar_core::Scalar ImageShiftScalar(
        blitzar_core::Scalar period, std::size_t axis_digit) noexcept;

    bool enabled_{false};
    blitzar_core::Vector3 origin_{};
    blitzar_core::Vector3 size_{};
};

} // namespace blitzar_physics

#endif
