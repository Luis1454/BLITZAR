#ifndef BLITZAR_TESTS_FIXTURES_FIXTURE_FORCE_HPP
#define BLITZAR_TESTS_FIXTURES_FIXTURE_FORCE_HPP

#include "solvers/SolverCpuForceProvider.hpp"

namespace blitzar_tests {

template <typename Solver>
[[nodiscard]] blitzar_status EvaluateLocal(Solver& solver,
    blitzar_core::ParticleStateView particles, blitzar_core::ForceView forces,
    const blitzar_core::ExecutionSettings& settings) noexcept
{
    blitzar_solvers::SolverCpuForceProvider<Solver> provider(solver);
    const blitzar_solvers::SolverForceEvaluation request{
        particles, particles, forces, settings, blitzar_solvers::SolverForceSourceKind::Local};

    return provider.Evaluate(request);
}

} // namespace blitzar_tests

#endif
