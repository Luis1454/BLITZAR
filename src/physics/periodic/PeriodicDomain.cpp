#include "physics/periodic/PeriodicDomain.hpp"

#include <cmath>

namespace blitzar_physics {

PeriodicDomain::PeriodicDomain(
    bool enabled, blitzar_core::Vector3 origin, blitzar_core::Vector3 size) noexcept
    : enabled_{enabled}, origin_{origin}, size_{size}
{
}

bool PeriodicDomain::IsEnabled() const noexcept
{
    return enabled_;
}

bool PeriodicDomain::IsValid() const noexcept
{
    if (!enabled_) {
        return true;
    }

    return std::isfinite(origin_.x) && std::isfinite(origin_.y) && std::isfinite(origin_.z) &&
           size_.x > 0.0 && size_.y > 0.0 && size_.z > 0.0 && std::isfinite(size_.x) &&
           std::isfinite(size_.y) && std::isfinite(size_.z);
}

blitzar_core::Scalar PeriodicDomain::WrapScalar(
    blitzar_core::Scalar value, blitzar_core::Scalar period) noexcept
{
    if (!(period > 0.0)) {
        return value;
    }

    return value - std::floor(value / period) * period;
}

blitzar_core::Scalar PeriodicDomain::FoldScalar(
    blitzar_core::Scalar value, blitzar_core::Scalar period) noexcept
{
    if (!(period > 0.0)) {
        return value;
    }

    return value - std::floor(value / period + 0.5) * period;
}

blitzar_core::Scalar PeriodicDomain::ImageShiftScalar(
    blitzar_core::Scalar period, std::size_t axis_digit) noexcept
{
    if (axis_digit == 1) {
        return period;
    }

    if (axis_digit == 2) {
        return -period;
    }

    return 0.0;
}

blitzar_core::Vector3 PeriodicDomain::Wrap(blitzar_core::Vector3 position) const noexcept
{
    return {origin_.x + WrapScalar(position.x - origin_.x, size_.x),
        origin_.y + WrapScalar(position.y - origin_.y, size_.y),
        origin_.z + WrapScalar(position.z - origin_.z, size_.z)};
}

blitzar_core::Vector3 PeriodicDomain::Fold(blitzar_core::Vector3 displacement) const noexcept
{
    return {FoldScalar(displacement.x, size_.x), FoldScalar(displacement.y, size_.y),
        FoldScalar(displacement.z, size_.z)};
}

bool PeriodicDomain::Contains(blitzar_core::Vector3 center, blitzar_core::Scalar half_extent,
    blitzar_core::Vector3 position) const noexcept
{
    blitzar_core::Scalar dx = position.x - center.x;
    blitzar_core::Scalar dy = position.y - center.y;
    blitzar_core::Scalar dz = position.z - center.z;

    if (enabled_) {
        dx = FoldScalar(dx, size_.x);
        dy = FoldScalar(dy, size_.y);
        dz = FoldScalar(dz, size_.z);
    }

    return std::abs(dx) <= half_extent && std::abs(dy) <= half_extent &&
           std::abs(dz) <= half_extent;
}

std::size_t PeriodicDomain::ImageCount() noexcept
{
    return 27;
}

blitzar_core::Vector3 PeriodicDomain::ImageShift(std::size_t index) const noexcept
{
    const std::size_t kx = index % 3;
    const std::size_t ky = (index / 3) % 3;
    const std::size_t kz = (index / 9) % 3;

    return {ImageShiftScalar(size_.x, kx), ImageShiftScalar(size_.y, ky),
        ImageShiftScalar(size_.z, kz)};
}

} // namespace blitzar_physics
