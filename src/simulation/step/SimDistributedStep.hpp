#ifndef BLITZAR_SIMULATION_STEP_SIM_DISTRIBUTED_STEP_HPP
#define BLITZAR_SIMULATION_STEP_SIM_DISTRIBUTED_STEP_HPP

#include "mpi/native/MpiNativeStatus.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "simulation/Sim.hpp"
#include "simulation/step/SimOverlapDispatch.hpp"
#include "simulation/transaction/SimTransaction.hpp"

#include <span>
#include <type_traits>

namespace blitzar_sim {

template <typename Solver> blitzar_status Sim::StepDistributed(Solver& solver) noexcept
{
    const std::size_t rollback_particle_count = particle_state_.Particles().Count();
    SimTransactionState transaction_state{particle_state_.Arena(), particle_state_.Particles(),
        particle_state_.Accelerations(), particle_state_.Checkpoint(), particle_ids_,
        local_particle_count_, exchange_buffer_, rollback_arena_buffer_, rollback_force_buffer_,
        rollback_exchange_buffer_};

    SimTransaction transaction(transaction_state);

    blitzar_status prepare_status = PrepareDistributedStep(rollback_particle_count, transaction);

    if (prepare_status != BLITZAR_STATUS_OK) {
        transaction.Abort();

        return prepare_status;
    }

    transaction.Begin();

    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = DistributedDispatcher<SolverType>;
    typename Dispatcher::DispatchContext dispatcher_context{
        {runtime_.Accelerator(), solver, gravity_, barnes_hut_, last_backend_}, runtime_.Exchange(),
        particle_source_, particle_ids_, exchange_buffer_,
        runtime_.Exchange().PersistentGhostExchange(), overlap_mode_, overlap_trace_};

    Dispatcher dispatcher(dispatcher_context);
    auto rollback = [&dispatcher, &transaction]() noexcept {
        dispatcher.Abort();
        transaction.Abort();
    };

    auto migrate_after_drift =
        [this, &solver, rollback_particle_count](
            blitzar_particles::ParticleBuffer& current_particles,
            blitzar_particles::ParticleAccelerationBuffer& current_accelerations,
            blitzar_integration::KdkCheckpoint& current_checkpoint)
        -> blitzar_integration_kdk::DriftTransition {
        const blitzar_integration_kdk::DriftTransition transition = MigrateAfterDrift(
            rollback_particle_count, current_particles, current_accelerations, current_checkpoint);

        if (transition.status != BLITZAR_STATUS_OK) {
            return transition;
        }

        const blitzar_status solver_status = solver.Prepare(current_particles.Count());
        const blitzar_status synchronized_solver_status = blitzar_parallel::SynchronizeStatus(
            runtime_.Mpi(), solver_status, "migrate-solver-capacity");

        return synchronized_solver_status == BLITZAR_STATUS_OK
                   ? transition
                   : blitzar_integration_kdk::DriftTransition{synchronized_solver_status, false};
    };

    blitzar_integration_kdk::AdvanceState<Dispatcher, blitzar_solver_threading::ThreadStackPool>
        advance_state{particle_state_.Particles(), particle_state_.Accelerations(),
            particle_state_.Checkpoint(), dispatcher, timestep_, execution_settings_,
            traversal_stacks_, particle_state_.Particles().State()};

    blitzar_integration_kdk::AdvanceHooks advance_hooks{migrate_after_drift, rollback};

    blitzar_integration_kdk::AdvanceRequest<Dispatcher, blitzar_solver_threading::ThreadStackPool,
        decltype(migrate_after_drift), decltype(rollback)>
        advance_request{advance_state, advance_hooks};

    const blitzar_status advance_status = integrator_.Advance(advance_request);

    if (advance_status != BLITZAR_STATUS_OK) {
        rollback();
    }
    else {
        transaction.Complete();
        transaction.Commit();
    }

    return advance_status;
}

} // namespace blitzar_sim

#endif
