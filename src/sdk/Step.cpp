#include "sdk/Simulation.hpp"

#include "sdk/Dispatch.hpp"
#include "sdk/State.hpp"
#include "sdk/Transaction.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <stdexcept>
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

template <typename Solver>
blitzar_status Simulation::StepLocal(Solver& solver) noexcept
{
    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = SolverDispatcher<SolverType>;

    Dispatcher dispatcher(
        SolverDispatchContext<SolverType>{hip_context_, solver, gravity_, barnes_hut_, last_backend_});

    blitzar_integration_kdk::AdvanceState<Dispatcher, blitzar_barnes_hut::ThreadStackPool>
        advance_state{particles_, accelerations_, checkpoint_, dispatcher, timestep_,
            execution_settings_, traversal_stacks_, particles_.State()};

    return integrator_.Advance(advance_state);
}

template <typename Solver>
blitzar_status Simulation::StepDistributed(Solver& solver) noexcept
{
    const std::size_t rollback_particle_count = particles_.Count();
    const std::size_t rollback_acceleration_count = accelerations_.Count();
    const std::size_t rollback_checkpoint_count = checkpoint_.Count();
    const bool rollback_state_valid =
        rollback_particle_count == local_particle_count_ &&
        rollback_particle_count <= particle_ids_.size() &&
        rollback_particle_count == rollback_acceleration_count &&
        rollback_particle_count == rollback_checkpoint_count &&
        rollback_particle_count <= arena_.Count();

    const blitzar_status state_status = SynchronizeSimulationStatus(mpi_context_,
        rollback_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "step-state");

    if (state_status != BLITZAR_STATUS_OK) {
        return state_status;
    }

    TransactionState transaction_state{
        arena_, particles_, accelerations_, checkpoint_,
        particle_ids_, local_particle_count_,
        exchange_buffer_, rollback_arena_buffer_, rollback_force_buffer_, rollback_exchange_buffer_};

    StepTransaction transaction(transaction_state);
    blitzar_status prepare_status = transaction.Prepare();

    prepare_status = SynchronizeSimulationStatus(mpi_context_, prepare_status, "step-prepare");

    if (prepare_status != BLITZAR_STATUS_OK) {
        transaction.Abort();

        return prepare_status;
    }

    transaction.Begin();

    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = DistributedDispatcher<SolverType>;
    typename Dispatcher::State dispatcher_state{
        {hip_context_, solver, gravity_, barnes_hut_, last_backend_}, mpi_exchange_,
        source_, particle_ids_, exchange_buffer_,
        mpi_exchange_.PersistentGhostExchange()};

    Dispatcher dispatcher(dispatcher_state);
    auto rollback = [&dispatcher, &transaction]() noexcept {
        dispatcher.Abort();
        transaction.Abort();
    };

    auto migrate_after_drift =
        [this, &solver, rollback_particle_count](
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
        const blitzar_status synchronized_solver_status = SynchronizeSimulationStatus(
            mpi_context_, solver_status, "migrate-solver-capacity");

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

blitzar_integration_kdk::DriftTransition Simulation::MigrateAfterDrift(
    std::size_t rollback_particle_count,
    blitzar_particles::ParticleBuffer& current_particles,
    blitzar_particles::AccelerationBuffer& current_accelerations,
    blitzar_integration::KdkCheckpoint& current_checkpoint) noexcept
{
    const bool migration_state_valid =
        current_particles.Count() == rollback_particle_count &&
        current_accelerations.Count() == rollback_particle_count &&
        current_checkpoint.Count() == rollback_particle_count &&
        local_particle_count_ == rollback_particle_count &&
        rollback_particle_count <= particle_ids_.size();

    blitzar_status migration_status = SynchronizeSimulationStatus(mpi_context_,
        migration_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR,
        "migrate-preflight");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    migration_status = mpi_exchange_.Migrate(current_particles.State(),
        std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
        migration_buffer_);

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    migration_status = migration_buffer_.Size() <= arena_.Count() &&
                               migration_buffer_.Size() <= particle_ids_.size()
                           ? BLITZAR_STATUS_OK
                           : BLITZAR_STATUS_INVALID_ARGUMENT;

    migration_status = SynchronizeSimulationStatus(
        mpi_context_, migration_status, "migrate-capacity");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    PacketStoreRequest migration_request{
        migration_buffer_, arena_, current_particles, current_accelerations, current_checkpoint,
        std::span<std::uint64_t>(particle_ids_), particle_count_, local_particle_count_};

    migration_status = StoreLocalPackets(migration_request);
    migration_status = SynchronizeSimulationStatus(
        mpi_context_, migration_status, "migrate-commit");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    (void)source_.SetCount(0);

    return {BLITZAR_STATUS_OK, true};
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
