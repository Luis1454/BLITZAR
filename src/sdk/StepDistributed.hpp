#ifndef BLITZAR_SDK_STEP_DISTRIBUTED_HPP
#define BLITZAR_SDK_STEP_DISTRIBUTED_HPP

#include "sdk/Dispatch.hpp"
#include "sdk/Simulation.hpp"
#include "sdk/State.hpp"
#include "sdk/Transaction.hpp"

#include <span>
#include <type_traits>

namespace blitzar_sdk {

template <typename Solver> blitzar_status Simulation::StepDistributed(Solver& solver) noexcept
{
    const std::size_t rollback_particle_count = particles_.Count();
    TransactionState transaction_state{arena_, particles_, accelerations_, checkpoint_,
        particle_ids_, local_particle_count_, exchange_buffer_, rollback_arena_buffer_,
        rollback_force_buffer_, rollback_exchange_buffer_};

    StepTransaction transaction(transaction_state);

    blitzar_status prepare_status = PrepareDistributedStep(rollback_particle_count, transaction);

    if (prepare_status != BLITZAR_STATUS_OK) {
        transaction.Abort();

        return prepare_status;
    }

    transaction.Begin();

    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = DistributedDispatcher<SolverType>;
    typename Dispatcher::State dispatcher_state{
        {hip_context_, solver, gravity_, barnes_hut_, last_backend_}, mpi_exchange_, source_,
        particle_ids_, exchange_buffer_, mpi_exchange_.PersistentGhostExchange(), overlap_mode_,
        overlap_trace_};

    Dispatcher dispatcher(dispatcher_state);
    auto rollback = [&dispatcher, &transaction]() noexcept {
        dispatcher.Abort();
        transaction.Abort();
    };

    auto migrate_after_drift = [this, &solver, rollback_particle_count](
                                   blitzar_particles::ParticleBuffer& current_particles,
                                   blitzar_particles::AccelerationBuffer& current_accelerations,
                                   blitzar_integration::KdkCheckpoint& current_checkpoint)
        -> blitzar_integration_kdk::DriftTransition {
        const blitzar_integration_kdk::DriftTransition transition = MigrateAfterDrift(
            rollback_particle_count, current_particles, current_accelerations, current_checkpoint);

        if (transition.status != BLITZAR_STATUS_OK) {
            return transition;
        }

        const blitzar_status solver_status = solver.Prepare(current_particles.Count());
        const blitzar_status synchronized_solver_status =
            SynchronizeSimulationStatus(mpi_context_, solver_status, "migrate-solver-capacity");

        return synchronized_solver_status == BLITZAR_STATUS_OK
                   ? transition
                   : blitzar_integration_kdk::DriftTransition{synchronized_solver_status, false};
    };

    blitzar_integration_kdk::AdvanceState<Dispatcher, blitzar_barnes_hut::ThreadStackPool>
        advance_state{particles_, accelerations_, checkpoint_, dispatcher, timestep_,
            execution_settings_, traversal_stacks_, particles_.State()};

    blitzar_integration_kdk::AdvanceHooks advance_hooks{migrate_after_drift, rollback};
    blitzar_integration_kdk::AdvanceRequest advance_request{advance_state, advance_hooks};
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

} // namespace blitzar_sdk

#endif
