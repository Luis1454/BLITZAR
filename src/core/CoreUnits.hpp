#ifndef BLITZAR_CORE_CORE_UNITS_HPP
#define BLITZAR_CORE_CORE_UNITS_HPP

#include "core/CoreTypes.hpp"

#include <cmath>

namespace blitzar_core {

struct UnitSystem final {
    Scalar length_scale{1.0};
    Scalar mass_scale{1.0};
    Scalar time_scale{1.0};

    [[nodiscard]] bool IsValid() const noexcept
    {
        return IsPositiveFinite(length_scale) && IsPositiveFinite(mass_scale) &&
               IsPositiveFinite(time_scale);
    }

private:
    [[nodiscard]] static bool IsPositiveFinite(Scalar value) noexcept
    {
        return std::isfinite(value) && value > 0.0;
    }
};

} // namespace blitzar_core

#endif
