#ifndef BLITZAR_SDK_STEP_LOCAL_HPP
#define BLITZAR_SDK_STEP_LOCAL_HPP

#include "sdk/Dispatch.hpp"
#include "sdk/Simulation.hpp"

#include <type_traits>

namespace blitzar_sdk {

template <typename Solver> blitzar_status Simulation::StepLocal(Solver& solver) noexcept
{
    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = SolverDispatcher<SolverType>;

    Dispatcher dispatcher(SolverDispatchContext<SolverType>{
        runtime_.Hip(), solver, gravity_, barnes_hut_, last_backend_});

    blitzar_integration_kdk::AdvanceState<Dispatcher, blitzar_barnes_hut::ThreadStackPool>
        advance_state{particle_storage_.Particles(), particle_storage_.Accelerations(),
            particle_storage_.Checkpoint(), dispatcher, timestep_, execution_settings_,
            traversal_stacks_, particle_storage_.Particles().State()};

    return integrator_.Advance(advance_state);
}

} // namespace blitzar_sdk

#endif
