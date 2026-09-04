#include "solvers/pm/PmSolver.hpp"

#include "fixtures/FixtureAllocationMonitor.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "solvers/SolverCpuForceProvider.hpp"
#include "solvers/SolverForceEvaluation.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace {

constexpr std::size_t Count = 6;

struct StateArrays final {
    std::array<double, Count> x{-1.2, -0.4, 0.1, 0.7, 1.3, 0.2};
    std::array<double, Count> y{-0.8, 0.6, -0.1, 0.9, -0.3, 0.2};
    std::array<double, Count> z{0.4, -0.2, 0.8, -0.7, 0.1, -0.5};
    std::array<double, Count> velocity_x{};
    std::array<double, Count> velocity_y{};
    std::array<double, Count> velocity_z{};
    std::array<double, Count> mass{1.0, 0.5, 1.5, 0.75, 2.0, 0.25};
};

[[nodiscard]] blitzar_core::ParticleStateView MakeState(const StateArrays& state) noexcept
{
    return {Count, state.x, state.y, state.z, state.velocity_x, state.velocity_y, state.velocity_z,
        state.mass};
}

[[nodiscard]] blitzar_core::ForceView MakeForces(std::array<double, Count>& x,
    std::array<double, Count>& y, std::array<double, Count>& z) noexcept
{
    return {Count, x, y, z};
}

[[nodiscard]] bool IsFinite(const std::array<double, Count>& x, const std::array<double, Count>& y,
    const std::array<double, Count>& z) noexcept
{
    for (std::size_t index = 0; index < Count; ++index) {
        if (!std::isfinite(x[index]) || !std::isfinite(y[index]) || !std::isfinite(z[index])) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] double RelativeError(const std::array<double, Count>& reference_x,
    const std::array<double, Count>& reference_y, const std::array<double, Count>& reference_z,
    const std::array<double, Count>& candidate_x, const std::array<double, Count>& candidate_y,
    const std::array<double, Count>& candidate_z) noexcept
{
    double difference = 0.0;
    double reference = 0.0;

    for (std::size_t index = 0; index < Count; ++index) {
        const double dx = candidate_x[index] - reference_x[index];
        const double dy = candidate_y[index] - reference_y[index];
        const double dz = candidate_z[index] - reference_z[index];

        difference += dx * dx + dy * dy + dz * dz;
        reference += reference_x[index] * reference_x[index] +
                     reference_y[index] * reference_y[index] +
                     reference_z[index] * reference_z[index];
    }

    return std::sqrt(difference / std::max(reference, 1.0e-24));
}

int EvaluateDirect(const blitzar_core::ParticleStateView state,
    const blitzar_core::ExecutionSettings& settings,
    const blitzar_physics::GravityParameters parameters, std::array<double, Count>& force_x,
    std::array<double, Count>& force_y, std::array<double, Count>& force_z)
{
    blitzar_direct::DirectSolver solver(parameters, Count);
    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> provider(solver);
    const blitzar_solvers::SolverForceEvaluation request{
        state, state, MakeForces(force_x, force_y, force_z), settings};

    BLITZAR_CHECK(provider.Evaluate(request) == BLITZAR_STATUS_OK);

    return 0;
}

int EvaluatePm(const blitzar_core::ParticleStateView state,
    const blitzar_core::ExecutionSettings& settings, blitzar_pm::PmSolver& solver,
    std::array<double, Count>& force_x, std::array<double, Count>& force_y,
    std::array<double, Count>& force_z)
{
    blitzar_solvers::SolverCpuForceProvider<blitzar_pm::PmSolver> provider(solver);
    const blitzar_solvers::SolverForceEvaluation request{
        state, state, MakeForces(force_x, force_y, force_z), settings};

    BLITZAR_CHECK(provider.Evaluate(request) == BLITZAR_STATUS_OK);

    return 0;
}

} // namespace

int main()
{
    const StateArrays state_arrays{};
    const blitzar_core::ParticleStateView state = MakeState(state_arrays);
    const blitzar_physics::GravityParameters parameters{1.0, 0.1, {}};
    const blitzar_core::ExecutionSettings settings{
        424242, blitzar_core::ExecutionMode::Deterministic};

    std::array<double, Count> direct_x{};
    std::array<double, Count> direct_y{};
    std::array<double, Count> direct_z{};
    std::array<double, Count> pm_x{};
    std::array<double, Count> pm_y{};
    std::array<double, Count> pm_z{};
    const blitzar_grid::GridResourceConfig grid_config{{8, 8, 8}, Count, 1.0};
    blitzar_pm::PmSolver solver(parameters, grid_config, Count);

    BLITZAR_CHECK(EvaluateDirect(state, settings, parameters, direct_x, direct_y, direct_z) == 0);
    BLITZAR_CHECK(EvaluatePm(state, settings, solver, pm_x, pm_y, pm_z) == 0);
    BLITZAR_CHECK(solver.Kind() == blitzar_solvers::SolverKind::Pm);
    BLITZAR_CHECK(solver.Resource().View().IsValid());
    BLITZAR_CHECK(IsFinite(pm_x, pm_y, pm_z));
    BLITZAR_CHECK(RelativeError(direct_x, direct_y, direct_z, pm_x, pm_y, pm_z) <= 4.0);

    const std::array<double, Count> first_x = pm_x;
    const std::array<double, Count> first_y = pm_y;
    const std::array<double, Count> first_z = pm_z;

    BLITZAR_CHECK(EvaluatePm(state, settings, solver, pm_x, pm_y, pm_z) == 0);
    BLITZAR_CHECK(pm_x == first_x);
    BLITZAR_CHECK(pm_y == first_y);
    BLITZAR_CHECK(pm_z == first_z);

    blitzar_tests::BeginAllocationCounting();

    BLITZAR_CHECK(EvaluatePm(state, settings, solver, pm_x, pm_y, pm_z) == 0);
    BLITZAR_CHECK(EvaluatePm(state, settings, solver, pm_x, pm_y, pm_z) == 0);

    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    BLITZAR_CHECK(allocations == 0);
    BLITZAR_CHECK(
        solver.Resource().BuildField(blitzar_physics::GravityLaw(parameters)) == BLITZAR_STATUS_OK);

    return 0;
}
