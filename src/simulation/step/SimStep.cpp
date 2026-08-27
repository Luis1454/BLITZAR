#include "mpi/native/MpiNativeStatus.hpp"
#include "simulation/Sim.hpp"
#include "simulation/step/SimDistributedStep.hpp"
#include "simulation/step/SimLocalStep.hpp"

#include <cmath>
#include <type_traits>
#include <utility>

namespace blitzar_sim {

template <typename Solver> blitzar_status Sim::StepWithSolver(Solver& solver) noexcept
{
    if (runtime_.Mpi().IsDistributed()) {
        return StepDistributed(solver);
    }

    return StepLocal(solver);
}

blitzar_status Sim::Step() noexcept
{
    const bool step_ready = particles_ready_ &&
                            integrator_kind_ == BLITZAR_INTEGRATOR_LEAPFROG_KDK &&
                            std::isfinite(timestep_) && timestep_ > 0.0;

    const blitzar_status preflight_status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(),
        step_ready ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "step-preflight");

    if (preflight_status != BLITZAR_STATUS_OK) {
        return Remember(preflight_status);
    }

    const blitzar_status status =
        std::visit([this](auto& solver) { return StepWithSolver(solver); }, solver_);

    return Remember(status);
}

} // namespace blitzar_sim
