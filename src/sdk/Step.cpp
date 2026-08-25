#include "sdk/Simulation.hpp"

#include "sdk/StepDistributed.hpp"
#include "sdk/StepLocal.hpp"
#include "sdk/State.hpp"

#include <cmath>
#include <type_traits>
#include <utility>

namespace blitzar_sdk {

template <typename Solver>
blitzar_status Simulation::StepWithSolver(Solver& solver) noexcept
{
    if (mpi_context_.IsDistributed()) {
        return StepDistributed(solver);
    }

    return StepLocal(solver);
}

blitzar_status Simulation::Step() noexcept
{
    const bool step_ready = particles_ready_ &&
                            integrator_kind_ == BLITZAR_INTEGRATOR_LEAPFROG_KDK &&
                            std::isfinite(timestep_) && timestep_ > 0.0;

    const blitzar_status preflight_status = SynchronizeSimulationStatus(mpi_context_,
        step_ready ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "step-preflight");

    if (preflight_status != BLITZAR_STATUS_OK) {
        return Remember(preflight_status);
    }

    const blitzar_status status = std::visit(
        [this](auto& solver) { return StepWithSolver(solver); }, solver_);

    return Remember(status);
}

} // namespace blitzar_sdk
