#include "integration/comoving/ComovingBackground.hpp"

#include <cmath>

namespace blitzar_integration {

blitzar_core::Scalar ScaleFactor(
    const ComovingBackground& background, blitzar_core::Scalar cosmic_time) noexcept
{
    if (background.kind == ExpansionKind::Static) {
        return 1.0;
    }

    return std::pow(cosmic_time / background.age_at_scale_one, 2.0 / 3.0);
}

blitzar_core::Scalar ExpansionRate(
    const ComovingBackground& background, blitzar_core::Scalar cosmic_time) noexcept
{
    if (background.kind == ExpansionKind::Static) {
        return 0.0;
    }

    return (2.0 / 3.0) / cosmic_time;
}

blitzar_core::Scalar Redshift(
    const ComovingBackground& background, blitzar_core::Scalar cosmic_time) noexcept
{
    if (background.kind == ExpansionKind::Static) {
        return 0.0;
    }

    const blitzar_core::Scalar scale_factor = ScaleFactor(background, cosmic_time);

    return 1.0 / scale_factor - 1.0;
}

blitzar_core::Scalar ComovingDrift(const ComovingBackground& background,
    blitzar_core::Scalar cosmic_time, blitzar_core::Scalar timestep) noexcept
{
    if (background.kind == ExpansionKind::Static) {
        return timestep;
    }

    const blitzar_core::Scalar scale_t0 = std::cbrt(background.age_at_scale_one);
    const blitzar_core::Scalar t0_scale = scale_t0 * scale_t0;

    return 3.0 * t0_scale * (std::cbrt(cosmic_time + timestep) - std::cbrt(cosmic_time));
}

} // namespace blitzar_integration
