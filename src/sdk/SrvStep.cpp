#include "sdk/Simulation.hpp"

#include "sdk/SrvDispatch.hpp"
#include "sdk/SrvState.hpp"
#include "sdk/SrvTransaction.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
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
    using Dispatcher = SrvSolverDispatcher<SolverType>;

    Dispatcher dispatcher(
        SrvSolverDispatchContext<SolverType>{hip_context_, solver, gravity_, barnes_hut_, last_backend_});

    blitzar_integration_kdk::AdvanceState<Dispatcher,
        blitzar_barnes_hut::ThreadWorkspace>
        advance_state{particles_, accelerations_, workspace_, dispatcher, timestep_,
            execution_settings_, traversal_workspace_, particles_.State()};

    return integrator_.Advance(advance_state);
}

template <typename Solver>
blitzar_status Simulation::StepDistributed(Solver& solver) noexcept
{
    const std::size_t rollback_particle_count = particles_.Count();
    const std::size_t rollback_acceleration_count = accelerations_.Count();
    const std::size_t rollback_workspace_count = workspace_.Count();
    const bool rollback_state_valid =
        rollback_particle_count == local_particle_count_ &&
        rollback_particle_count <= particle_ids_.size() &&
        rollback_particle_count == rollback_acceleration_count &&
        rollback_particle_count == rollback_workspace_count &&
        rollback_particle_count <= source_particle_count_ &&
        source_particle_count_ <= arena_.Count();

    const blitzar_status state_status = SynchronizeSimulationStatus(mpi_context_,
        rollback_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "step-state");

    if (state_status != BLITZAR_STATUS_OK) {
        return state_status;
    }

    SrvTransactionState transaction_state{
        arena_, particles_, accelerations_, workspace_,
        std::span<std::uint64_t>(particle_ids_), local_particle_count_, source_particle_count_,
        exchange_buffer_, rollback_arena_buffer_, rollback_force_buffer_, rollback_exchange_buffer_};

    SrvStepTransaction transaction(transaction_state);
    blitzar_status prepare_status = transaction.Prepare();

    prepare_status = SynchronizeSimulationStatus(mpi_context_, prepare_status, "step-prepare");

    if (prepare_status != BLITZAR_STATUS_OK) {
        transaction.Abort();

        return prepare_status;
    }

    transaction.Begin();

    using SolverType = std::remove_reference_t<decltype(solver)>;
    using Dispatcher = SrvDistributedDispatcher<SolverType>;
    typename Dispatcher::State dispatcher_state{
        {hip_context_, solver, gravity_, barnes_hut_, last_backend_}, mpi_exchange_,
        arena_, std::span<const std::uint64_t>(particle_ids_), exchange_buffer_,
        mpi_exchange_.PersistentGhostExchange(), source_particle_count_};

    Dispatcher dispatcher(dispatcher_state);
    auto rollback = [&dispatcher, &transaction]() noexcept {
        dispatcher.Abort();
        transaction.Abort();
    };

    auto migrate_after_drift =
        [this, rollback_particle_count](blitzar_particles::ParticleBuffer& current_particles,
            blitzar_particles::AccelerationBuffer& current_accelerations,
            blitzar_integration::LeapfrogWorkspace& current_workspace)
        -> blitzar_integration_kdk::DriftTransition {
        return MigrateAfterDrift(
            rollback_particle_count, current_particles, current_accelerations, current_workspace);
    };

    blitzar_integration_kdk::AdvanceState<Dispatcher,
        blitzar_barnes_hut::ThreadWorkspace>
        advance_state{particles_, accelerations_, workspace_, dispatcher, timestep_,
            execution_settings_, traversal_workspace_, particles_.State()};

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
    blitzar_integration::LeapfrogWorkspace& current_workspace) noexcept
{
    const bool migration_state_valid =
        current_particles.Count() == rollback_particle_count &&
        current_accelerations.Count() == rollback_particle_count &&
        current_workspace.Count() == rollback_particle_count &&
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

    SrvPacketStoreRequest migration_request{
        migration_buffer_, arena_, current_particles, current_accelerations, current_workspace,
        std::span<std::uint64_t>(particle_ids_), particle_count_, local_particle_count_};

    migration_status = SrvStoreLocalPackets(migration_request);
    migration_status = SynchronizeSimulationStatus(
        mpi_context_, migration_status, "migrate-commit");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    source_particle_count_ = local_particle_count_;

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
