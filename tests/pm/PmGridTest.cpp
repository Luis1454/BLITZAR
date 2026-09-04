#include "fixtures/FixtureCheck.hpp"
#include "grid/GridResource.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

constexpr std::size_t Count = 4;

struct StateArrays final {
    std::array<double, Count> x{-0.75, 0.6, 0.15, -0.2};
    std::array<double, Count> y{-0.5, -0.25, 0.7, 0.4};
    std::array<double, Count> z{0.2, -0.6, 0.35, -0.1};
    std::array<double, Count> velocity_x{};
    std::array<double, Count> velocity_y{};
    std::array<double, Count> velocity_z{};
    std::array<double, Count> mass{1.0, 2.0, 0.5, 1.5};
};

[[nodiscard]] blitzar_core::ParticleStateView MakeState(const StateArrays& state) noexcept
{
    return {Count, state.x, state.y, state.z, state.velocity_x, state.velocity_y, state.velocity_z,
        state.mass};
}

[[nodiscard]] bool IsFinite(blitzar_core::Vector3 value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

int CheckInitialAndPreparedView(
    blitzar_grid::GridResource& resource, blitzar_core::ParticleStateView state)
{
    const blitzar_grid::GridView initial = resource.View();

    BLITZAR_CHECK(!initial.IsValid());
    BLITZAR_CHECK(!resource.IsCurrent(initial));
    BLITZAR_CHECK(resource.Prepare(state) == BLITZAR_STATUS_OK);

    const blitzar_grid::GridView prepared = resource.View();

    BLITZAR_CHECK(prepared.IsValid());
    BLITZAR_CHECK(resource.IsCurrent(prepared));
    BLITZAR_CHECK(prepared.CellCount() == 512);
    BLITZAR_CHECK(prepared.Dimensions().x == 8);
    BLITZAR_CHECK(prepared.Dimensions().y == 8);
    BLITZAR_CHECK(prepared.Dimensions().z == 8);

    blitzar_core::Scalar deposited_mass = 0.0;

    for (const blitzar_core::Scalar value : prepared.Density()) {
        BLITZAR_CHECK(std::isfinite(value));
        BLITZAR_CHECK(value >= 0.0);

        deposited_mass += value;
    }

    blitzar_core::Scalar source_mass = 0.0;

    for (const blitzar_core::Scalar value : state.mass) {
        source_mass += value;
    }

    BLITZAR_CHECK(std::abs(deposited_mass - source_mass) <= 1.0e-12);

    return 0;
}

int CheckBoundaryAndRepeatability(
    blitzar_grid::GridResource& resource, blitzar_core::ParticleStateView state)
{
    const blitzar_grid::GridView first = resource.View();
    const blitzar_grid::GridLayout& layout = first.Layout();
    blitzar_grid::GridInterpolation minimum_location{};
    blitzar_grid::GridInterpolation maximum_location{};

    BLITZAR_CHECK(layout.Locate(layout.Minimum(), minimum_location));
    BLITZAR_CHECK(layout.Locate(layout.Maximum(), maximum_location));
    BLITZAR_CHECK(minimum_location.lower.x == 0);
    BLITZAR_CHECK(minimum_location.lower.y == 0);
    BLITZAR_CHECK(minimum_location.lower.z == 0);
    BLITZAR_CHECK(maximum_location.lower.x == 7);
    BLITZAR_CHECK(maximum_location.lower.y == 7);
    BLITZAR_CHECK(maximum_location.lower.z == 7);

    const std::vector<blitzar_core::Scalar> density(first.Density().begin(), first.Density().end());

    BLITZAR_CHECK(resource.Prepare(state) == BLITZAR_STATUS_OK);

    const blitzar_grid::GridView second = resource.View();

    BLITZAR_CHECK(!resource.IsCurrent(first));
    BLITZAR_CHECK(resource.IsCurrent(second));
    BLITZAR_CHECK(second.Generation() != first.Generation());
    BLITZAR_CHECK(std::equal(density.begin(), density.end(), second.Density().begin()));

    return 0;
}

int CheckFieldAndCapacity(
    blitzar_grid::GridResource& resource, blitzar_core::ParticleStateView state)
{
    const blitzar_physics::GravityParameters parameters{1.0, 0.1, {}};
    const blitzar_physics::GravityLaw gravity(parameters);

    BLITZAR_CHECK(resource.BuildField(gravity) == BLITZAR_STATUS_OK);

    const blitzar_grid::GridView view = resource.View();

    for (std::size_t index = 0; index < view.CellCount(); ++index) {
        const blitzar_core::Vector3 field{
            view.FieldX()[index], view.FieldY()[index], view.FieldZ()[index]};

        BLITZAR_CHECK(IsFinite(field));
    }

    std::array<double, Count + 1> extra_x{0.0, 1.0, 2.0, 3.0, 4.0};
    const blitzar_core::ParticleStateView oversized_state{Count + 1, extra_x,
        std::array<double, Count + 1>{}, std::array<double, Count + 1>{},
        std::array<double, Count + 1>{}, std::array<double, Count + 1>{},
        std::array<double, Count + 1>{}, std::array<double, Count + 1>{1.0, 1.0, 1.0, 1.0, 1.0}};

    BLITZAR_CHECK(resource.Prepare(oversized_state) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(resource.IsCurrent(view));
    BLITZAR_CHECK(blitzar_core::IsValid(state));

    return 0;
}

} // namespace

int main()
{
    const blitzar_grid::GridResourceConfig config{{8, 8, 8}, Count, 1.0};
    blitzar_grid::GridResource resource(config);
    const StateArrays state{};
    const blitzar_core::ParticleStateView view = MakeState(state);

    BLITZAR_CHECK(config.IsValid());
    BLITZAR_CHECK(CheckInitialAndPreparedView(resource, view) == 0);
    BLITZAR_CHECK(CheckBoundaryAndRepeatability(resource, view) == 0);
    BLITZAR_CHECK(CheckFieldAndCapacity(resource, view) == 0);

    return 0;
}
