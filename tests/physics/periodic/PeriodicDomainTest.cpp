#include "physics/periodic/PeriodicDomain.hpp"

#include "core/CoreTypes.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

namespace {

[[nodiscard]] bool RunVisibilityCase() noexcept
{
    const blitzar_physics::PeriodicDomain disabled{};
    const blitzar_physics::PeriodicDomain enabled{
        true, blitzar_core::Vector3{1.0, -2.0, 0.5}, blitzar_core::Vector3{2.0, 3.0, 4.0}};

    if (!disabled.IsValid() || disabled.IsEnabled() || !enabled.IsValid() || !enabled.IsEnabled()) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunWrappingCase() noexcept
{
    const blitzar_physics::PeriodicDomain domain{
        true, blitzar_core::Vector3{1.0, -2.0, 0.5}, blitzar_core::Vector3{2.0, 4.0, 8.0}};

    const blitzar_core::Vector3 wrapped = domain.Wrap(blitzar_core::Vector3{3.0, -6.0, 8.5});

    if (wrapped.x != 1.0 || wrapped.y != -2.0 || wrapped.z != 0.5) {
        return false;
    }

    const blitzar_core::Vector3 shifted{wrapped.x + 2.0, wrapped.y + 4.0, wrapped.z + 8.0};

    if (domain.Wrap(shifted).x != wrapped.x || domain.Wrap(shifted).y != wrapped.y ||
        domain.Wrap(shifted).z != wrapped.z) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunFoldCase() noexcept
{
    const blitzar_physics::PeriodicDomain domain{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_core::Vector3 folded = domain.Fold(blitzar_core::Vector3{1.5, -1.5, 0.2});

    if (folded.x != -0.5 || folded.y != 0.5 || folded.z != 0.2) {
        return false;
    }

    const blitzar_core::Vector3 in_range{0.25, -0.25, 0.5};

    if (domain.Fold(in_range).x != in_range.x || domain.Fold(in_range).y != in_range.y ||
        domain.Fold(in_range).z != in_range.z) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunDisabledIdentityCase() noexcept
{
    const blitzar_physics::PeriodicDomain disabled{};

    const blitzar_core::Vector3 position{0.3, -1.7, 2.1};
    const blitzar_core::Vector3 displacement{0.7, -0.9, 0.2};

    if (disabled.Wrap(position).x != position.x || disabled.Wrap(position).y != position.y ||
        disabled.Wrap(position).z != position.z) {
        return false;
    }

    if (disabled.Fold(displacement).x != displacement.x ||
        disabled.Fold(displacement).y != displacement.y ||
        disabled.Fold(displacement).z != displacement.z) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunImageRegionCase() noexcept
{
    const blitzar_physics::PeriodicDomain domain{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    if (domain.ImageCount() != 27) {
        return false;
    }

    const blitzar_core::Vector3 zero{0.0, 0.0, 0.0};

    if (domain.ImageShift(0).x != zero.x || domain.ImageShift(0).y != zero.y ||
        domain.ImageShift(0).z != zero.z) {
        return false;
    }

    if (domain.ImageShift(1).x != 2.0 || domain.ImageShift(1).y != 0.0 ||
        domain.ImageShift(1).z != 0.0) {
        return false;
    }

    if (domain.ImageShift(2).x != -2.0 || domain.ImageShift(2).y != 0.0 ||
        domain.ImageShift(2).z != 0.0) {
        return false;
    }

    if (domain.ImageShift(26).x != -2.0 || domain.ImageShift(26).y != -2.0 ||
        domain.ImageShift(26).z != -2.0) {
        return false;
    }

    const blitzar_core::Vector3 first{0.3, 0.1, -0.2};
    const blitzar_core::Vector3 second{-0.4, 0.2, 0.1};

    const blitzar_core::Vector3 displacement{
        first.x - second.x, first.y - second.y, first.z - second.z};

    const blitzar_core::Vector3 folded = domain.Fold(displacement);
    std::size_t best_index = 0;
    double best_norm = 1.0e30;

    for (std::size_t index = 0; index < domain.ImageCount(); ++index) {
        const blitzar_core::Vector3 shift = domain.ImageShift(index);
        const double dx = displacement.x - shift.x;
        const double dy = displacement.y - shift.y;
        const double dz = displacement.z - shift.z;
        const double norm = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (norm < best_norm) {
            best_norm = norm;
            best_index = index;
        }
    }

    const blitzar_core::Vector3 best_shift = domain.ImageShift(best_index);

    if (std::abs((displacement.x - best_shift.x) - folded.x) > 1.0e-12 ||
        std::abs((displacement.y - best_shift.y) - folded.y) > 1.0e-12 ||
        std::abs((displacement.z - best_shift.z) - folded.z) > 1.0e-12) {
        return false;
    }

    std::size_t minimum_count = 0;

    for (std::size_t index = 0; index < domain.ImageCount(); ++index) {
        const blitzar_core::Vector3 shift = domain.ImageShift(index);
        const double dx = displacement.x - shift.x;
        const double dy = displacement.y - shift.y;
        const double dz = displacement.z - shift.z;
        const double norm = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (std::abs(norm - best_norm) < 1.0e-9) {
            ++minimum_count;
        }
    }

    if (minimum_count != 1) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunContainmentCase() noexcept
{
    const blitzar_physics::PeriodicDomain domain{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_core::Vector3 center{0.95, 0.0, 0.0};

    if (!domain.Contains(center, 0.05, blitzar_core::Vector3{0.975, 0.0, 0.0})) {
        return false;
    }

    if (domain.Contains(center, 0.05, blitzar_core::Vector3{0.3, 0.0, 0.0})) {
        return false;
    }

    const blitzar_physics::PeriodicDomain disabled{};
    const blitzar_core::Vector3 other{0.75, 0.5, -0.4};

    if (!disabled.Contains(other, 0.4, blitzar_core::Vector3{0.9, 0.3, -0.3})) {
        return false;
    }

    if (disabled.Contains(other, 0.4, blitzar_core::Vector3{1.2, 0.3, -0.3})) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunValidationCase() noexcept
{
    const double nan = std::numeric_limits<double>::quiet_NaN();

    const blitzar_physics::PeriodicDomain invalid_origin{
        true, blitzar_core::Vector3{nan, 0.0, 0.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_physics::PeriodicDomain invalid_size{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{-2.0, 2.0, 2.0}};

    const blitzar_physics::PeriodicDomain zero_size{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{0.0, 2.0, 2.0}};

    if (invalid_origin.IsValid() || invalid_size.IsValid() || zero_size.IsValid()) {
        return false;
    }

    return true;
}

} // namespace

int main()
{
    BLITZAR_CHECK(RunVisibilityCase());
    BLITZAR_CHECK(RunWrappingCase());
    BLITZAR_CHECK(RunFoldCase());
    BLITZAR_CHECK(RunDisabledIdentityCase());
    BLITZAR_CHECK(RunImageRegionCase());
    BLITZAR_CHECK(RunContainmentCase());
    BLITZAR_CHECK(RunValidationCase());

    return 0;
}
