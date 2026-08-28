#ifndef BLITZAR_SOLVERS_SOLVER_FORCE_EVALUATION_HPP
#define BLITZAR_SOLVERS_SOLVER_FORCE_EVALUATION_HPP

#include "core/CoreExecution.hpp"
#include "core/CoreTypes.hpp"

#include <blitzar/blitzar.h>
#include <concepts>
#include <cstdint>

namespace blitzar_solvers {

enum class SolverForceSourceKind : std::uint8_t {
    Local,
    Remote,
};

struct SolverForceEvaluation final {
    blitzar_core::ParticleStateView targets;
    blitzar_core::ParticleStateView sources;
    blitzar_core::ForceView forces;
    const blitzar_core::ExecutionSettings& settings;
    SolverForceSourceKind source_kind{SolverForceSourceKind::Local};
};

template <typename Provider>
concept SolverForceProvider = requires(Provider& provider, const SolverForceEvaluation& request) {
    { provider.Evaluate(request) } -> std::same_as<blitzar_status>;
};

} // namespace blitzar_solvers

#endif
