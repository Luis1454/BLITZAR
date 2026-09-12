#include "core/CoreExecution.hpp"
#include "core/CoreTypes.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "solvers/SolverCpuForceProvider.hpp"
#include "solvers/SolverForceEvaluation.hpp"
#include "solvers/SolverForceRequest.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <array>
#include <cmath>
#include <type_traits>

static_assert(std::is_aggregate_v<blitzar_solvers::SolverForceEvaluation>);
static_assert(std::is_aggregate_v<blitzar_solvers::SolverForceRequest::Direct>);
static_assert(blitzar_solvers::SolverForceProvider<
    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver>>);

int main()
{
    std::array<blitzar_core::Scalar, 1> target_x{0.0};
    std::array<blitzar_core::Scalar, 1> target_y{};
    std::array<blitzar_core::Scalar, 1> target_z{};
    std::array<blitzar_core::Scalar, 1> target_velocity_x{};
    std::array<blitzar_core::Scalar, 1> target_velocity_y{};
    std::array<blitzar_core::Scalar, 1> target_velocity_z{};
    std::array<blitzar_core::Scalar, 1> target_mass{1.0};
    std::array<blitzar_core::Scalar, 2> source_x{0.0, 1.0};
    std::array<blitzar_core::Scalar, 2> source_y{};
    std::array<blitzar_core::Scalar, 2> source_z{};
    std::array<blitzar_core::Scalar, 2> source_velocity_x{};
    std::array<blitzar_core::Scalar, 2> source_velocity_y{};
    std::array<blitzar_core::Scalar, 2> source_velocity_z{};
    std::array<blitzar_core::Scalar, 2> source_mass{0.0, 1.0};
    std::array<blitzar_core::Scalar, 1> force_x{};
    std::array<blitzar_core::Scalar, 1> force_y{};
    std::array<blitzar_core::Scalar, 1> force_z{};

    const blitzar_core::ParticleStateView target{1, target_x, target_y, target_z, target_velocity_x,
        target_velocity_y, target_velocity_z, target_mass};

    const blitzar_core::ParticleStateView source{2, source_x, source_y, source_z, source_velocity_x,
        source_velocity_y, source_velocity_z, source_mass};

    const blitzar_core::ForceView force{1, force_x, force_y, force_z};
    const blitzar_core::ExecutionSettings settings{};
    blitzar_direct::DirectSolver solver({1.0, 0.0}, 1);
    blitzar_solvers::SolverCpuForceProvider provider(solver);

    const blitzar_solvers::SolverForceEvaluation local_request{
        target, source, force, settings, blitzar_solvers::SolverForceSourceKind::Local};

    BLITZAR_CHECK(provider.Evaluate(local_request) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(std::abs(force.x[0] - 1.0) < 1.0e-12);

    force_x[0] = 3.0;

    const blitzar_solvers::SolverForceEvaluation remote_request{
        target, source, force, settings, blitzar_solvers::SolverForceSourceKind::Remote};

    BLITZAR_CHECK(provider.Evaluate(remote_request) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(std::abs(force.x[0] - 4.0) < 1.0e-12);

    const blitzar_solvers::SolverForceRequest::Direct typed_request{
        target, source, force, settings, {0, source.SourceCount(), false}, true};

    BLITZAR_CHECK(solver.Evaluate(typed_request) == BLITZAR_STATUS_OK);

    return 0;
}
