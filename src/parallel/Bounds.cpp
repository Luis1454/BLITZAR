#include "parallel/Bounds.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace blitzar_parallel {

namespace {

constexpr blitzar_core::Scalar PositiveInfinity =
    std::numeric_limits<blitzar_core::Scalar>::infinity();

[[nodiscard]] bool IsFinite(blitzar_core::Vector3 position) noexcept
{
    return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
}

} // namespace

bool DomainBounds::IsValid() const noexcept
{
    return IsFinite(minimum) && IsFinite(maximum) && minimum.x <= maximum.x &&
           minimum.y <= maximum.y && minimum.z <= maximum.z;
}

bool DomainBounds::Contains(blitzar_core::Vector3 position) const noexcept
{
    return IsValid() && IsFinite(position) && position.x >= minimum.x && position.x <= maximum.x &&
           position.y >= minimum.y && position.y <= maximum.y && position.z >= minimum.z &&
           position.z <= maximum.z;
}

bool DomainBounds::Include(blitzar_core::Vector3 position) noexcept
{
    if (!IsFinite(position)) {
        return false;
    }

    minimum.x = std::min(minimum.x, position.x);
    minimum.y = std::min(minimum.y, position.y);
    minimum.z = std::min(minimum.z, position.z);
    maximum.x = std::max(maximum.x, position.x);
    maximum.y = std::max(maximum.y, position.y);
    maximum.z = std::max(maximum.z, position.z);

    return true;
}

DomainBounds DomainBounds::From(blitzar_core::ParticleStateView state) noexcept
{
    DomainBounds bounds{{PositiveInfinity, PositiveInfinity, PositiveInfinity},
        {-PositiveInfinity, -PositiveInfinity, -PositiveInfinity}};

    for (std::size_t index = 0; index < state.SourceCount(); ++index) {
        (void)bounds.Include({state.x[index], state.y[index], state.z[index]});
    }

    return bounds.IsValid() ? bounds : DomainBounds{};
}

} // namespace blitzar_parallel
