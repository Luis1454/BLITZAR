#ifndef BLITZAR_SIMULATION_STEP_SIM_LOCAL_STEP_HPP
#define BLITZAR_SIMULATION_STEP_SIM_LOCAL_STEP_HPP

#include "simulation/Sim.hpp"
#include "simulation/solver/SimSolverDispatch.hpp"

#include <type_traits>

namespace blitzar_sim {

template <typename Solver> blitzar_status Sim::StepLocal(Solver& solver) noexcept
{
    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = SolverDispatcher<SolverType>;

    Dispatcher dispatcher(SolverDispatchContext<SolverType>{
        runtime_.Accelerator(), solver, gravity_, barnes_hut_, last_backend_});

    blitzar_integration_kdk::AdvanceState<Dispatcher, blitzar_solver_threading::ThreadStackPool>
        advance_state{particle_state_.Particles(), particle_state_.Accelerations(),
            particle_state_.Checkpoint(), dispatcher, timestep_, execution_settings_,
            traversal_stacks_, particle_state_.Particles().State()};

    return integrator_.Advance(advance_state);
}

} // namespace blitzar_sim

#endif
