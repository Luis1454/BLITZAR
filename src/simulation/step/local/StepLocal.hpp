#ifndef BLITZAR_SIMULATION_STEP_LOCAL_STEP_LOCAL_HPP
#define BLITZAR_SIMULATION_STEP_LOCAL_STEP_LOCAL_HPP

#include "simulation/composition/Composition.hpp"
#include "simulation/facade/Simulation.hpp"

#include <type_traits>

namespace blitzar_sim {

template <typename Solver> blitzar_status Simulation::StepLocal(Solver& solver) noexcept
{
    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = SolverDispatcher<SolverType>;

    Dispatcher dispatcher(SolverDispatchContext<SolverType>{
        resources_.Accelerator(), solver, gravity_, barnes_hut_, last_backend_});

    blitzar_integration_kdk::AdvanceState<Dispatcher, blitzar_solver_threading::ThreadStackPool>
        advance_state{particle_storage_.Particles(), particle_storage_.Accelerations(),
            particle_storage_.Checkpoint(), dispatcher, timestep_, execution_settings_,
            traversal_stacks_, particle_storage_.Particles().State()};

    return integrator_.Advance(advance_state);
}

} // namespace blitzar_sim

#endif
