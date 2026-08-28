#ifndef BLITZAR_SIMULATION_STEP_SIM_LOCAL_STEP_HPP
#define BLITZAR_SIMULATION_STEP_SIM_LOCAL_STEP_HPP

#include "simulation/Sim.hpp"
#include "simulation/solver/SimBackendForceProvider.hpp"

namespace blitzar_sim {

template <typename Solver> blitzar_status Sim::StepLocal(Solver& solver) noexcept
{
    SimBackendForceProvider<Solver> force_provider(
        {runtime_.Accelerator(), solver, gravity_, barnes_hut_, last_backend_});

    blitzar_integration_kdk::AdvanceState<SimBackendForceProvider<Solver>> advance_state{
        particle_state_.Particles(), particle_state_.Accelerations(), particle_state_.Checkpoint(),
        force_provider, timestep_, execution_settings_, particle_state_.Particles().State()};

    return integrator_.Advance(advance_state);
}

} // namespace blitzar_sim

#endif
