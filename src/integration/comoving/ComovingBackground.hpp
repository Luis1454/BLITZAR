#ifndef BLITZAR_INTEGRATION_COMOVING_COMOVING_BACKGROUND_HPP
#define BLITZAR_INTEGRATION_COMOVING_COMOVING_BACKGROUND_HPP

#include "core/CoreExecution.hpp"
#include "core/CoreTypes.hpp"

#include <cmath>
#include <cstdint>

namespace blitzar_integration {

enum class ExpansionKind : std::uint8_t { Static, EinsteinDeSitter };

struct ComovingBackground final {
    ExpansionKind kind{ExpansionKind::Static};
    blitzar_core::Scalar age_at_scale_one{1.0};

    [[nodiscard]] bool IsValid() const noexcept
    {
        return (kind == ExpansionKind::Static || kind == ExpansionKind::EinsteinDeSitter) &&
               std::isfinite(age_at_scale_one) && age_at_scale_one > 0.0;
    }
};

[[nodiscard]] blitzar_core::Scalar ScaleFactor(
    const ComovingBackground& background, blitzar_core::Scalar cosmic_time) noexcept;

[[nodiscard]] blitzar_core::Scalar ExpansionRate(
    const ComovingBackground& background, blitzar_core::Scalar cosmic_time) noexcept;

[[nodiscard]] blitzar_core::Scalar Redshift(
    const ComovingBackground& background, blitzar_core::Scalar cosmic_time) noexcept;

[[nodiscard]] blitzar_core::Scalar ComovingDrift(const ComovingBackground& background,
    blitzar_core::Scalar cosmic_time, blitzar_core::Scalar timestep) noexcept;

} // namespace blitzar_integration

#endif
