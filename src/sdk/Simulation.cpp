#include "sdk/Simulation.hpp"

#include "sdk/SrvDispatch.hpp"
#include "sdk/SrvState.hpp"
#include "sdk/SrvTransaction.hpp"

#include "particles/ParticleArena.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace blitzar_sdk {

struct Simulation::SolverCreationRequest final {
    blitzar_solver_kind solver_kind{BLITZAR_SOLVER_DIRECT};
    blitzar_physics::GravityParameters gravity{};
    blitzar_barnes_hut::BarnesHutSettings barnes_hut{};
    std::size_t staging_capacity{};
};

std::size_t Simulation::DefaultMaxCells(std::size_t particle_count) noexcept
{
    if (particle_count == 0) {
        return 1;
    }

    const std::size_t maximum = std::numeric_limits<std::size_t>::max();

    if (particle_count > (maximum - 1) / 8) {
        return 0;
    }

    return particle_count * 8 + 1;
}

Simulation::Simulation(std::size_t particle_count)
    : particle_count_(particle_count), mpi_context_(), domain_(),
      mpi_exchange_(mpi_context_, domain_, particle_count), hip_context_(), arena_(particle_count),
      particles_(arena_), accelerations_(arena_), workspace_(arena_), gravity_{},
      barnes_hut_{
          0.5, particle_count == 0 ? 1 : particle_count, DefaultMaxCells(particle_count), 8, 32},
      traversal_workspace_(barnes_hut_.max_cells, barnes_hut_.max_depth),
      solver_kind_(BLITZAR_SOLVER_DIRECT), integrator_kind_(BLITZAR_INTEGRATOR_LEAPFROG_KDK),
      timestep_(1.0), particles_ready_(false), execution_settings_{}, snapshot_header_{},
      last_status_(mpi_context_.Status()), last_backend_(BLITZAR_BACKEND_CPU),
      solver_(std::in_place_type<blitzar_direct::DirectSolver>, gravity_, particle_count),
      integrator_{}, particle_ids_(particle_count), local_particle_count_(particle_count),
      source_particle_count_(particle_count), exchange_buffer_{}, rollback_arena_buffer_{},
      rollback_force_buffer_{}, rollback_exchange_buffer_{}, migration_buffer_{},
      gathered_buffer_{}, local_indices_{}, seen_{}
{
    const blitzar_status capacity_status = mpi_exchange_.CapacityStatus();
    if (capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE) {
        throw std::bad_alloc();
    }
    if (capacity_status != BLITZAR_STATUS_OK) {
        throw std::length_error("simulation capacity preparation failed");
    }
    exchange_buffer_.Reserve(particle_count_);
    rollback_arena_buffer_.Reserve(particle_count_);
    rollback_force_buffer_.Reserve(particle_count_);
    rollback_exchange_buffer_.Reserve(particle_count_);
    migration_buffer_.Reserve(particle_count_);
    gathered_buffer_.Reserve(particle_count_);
    local_indices_.reserve(particle_count_);
    seen_.resize(particle_count_);
    snapshot_header_.particle_count = particle_count_;
}

blitzar_status Simulation::LastStatus() const noexcept
{
    return last_status_.load(std::memory_order_relaxed);
}

blitzar_backend_kind Simulation::LastBackend() const noexcept
{
    return last_backend_.load(std::memory_order_relaxed);
}

void Simulation::SetHipFaultForTesting(blitzar_gpu::HipFault fault) noexcept
{
    hip_context_.SetFaultForTesting(fault);
}

std::size_t Simulation::ParticleCount() const noexcept
{
    return particle_count_;
}

blitzar_status Simulation::CreateSolver(
    const SolverCreationRequest& request, SolverVariant& solver) noexcept
{
    try {
        switch (request.solver_kind) {
        case BLITZAR_SOLVER_DIRECT:

            solver.emplace<blitzar_direct::DirectSolver>(request.gravity, request.staging_capacity);

            return BLITZAR_STATUS_OK;

        case BLITZAR_SOLVER_BARNES_HUT:

            if (!request.barnes_hut.IsValid()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            solver.emplace<blitzar_barnes_hut::BarnesHutSolver>(
                request.gravity, request.barnes_hut);

            return BLITZAR_STATUS_OK;

        case BLITZAR_SOLVER_FMM:
        case BLITZAR_SOLVER_PM:
        case BLITZAR_SOLVER_TREEPM:

            return BLITZAR_STATUS_UNSUPPORTED;

        default:

            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status Simulation::Remember(blitzar_status status) const noexcept
{
    last_status_.store(status, std::memory_order_relaxed);

    return status;
}

blitzar_status Simulation::SetSolver(blitzar_solver_kind solver) noexcept
{
    SolverVariant candidate(std::in_place_type<blitzar_direct::DirectSolver>, gravity_);
    const SolverCreationRequest request{solver, gravity_, barnes_hut_, particle_count_};
    const blitzar_status status = CreateSolver(request, candidate);

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    solver_kind_ = solver;
    solver_ = std::move(candidate);

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetIntegrator(blitzar_integrator_kind integrator) noexcept
{
    if (integrator != BLITZAR_INTEGRATOR_LEAPFROG_KDK) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    integrator_kind_ = integrator;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetGravity(
    blitzar_core::Scalar gravitational_constant, blitzar_core::Scalar softening) noexcept
{
    const blitzar_physics::GravityParameters candidate_parameters{
        gravitational_constant, softening, gravity_.units};

    if (!candidate_parameters.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    SolverVariant candidate_solver(
        std::in_place_type<blitzar_direct::DirectSolver>, candidate_parameters);

    const SolverCreationRequest request{
        solver_kind_, candidate_parameters, barnes_hut_, particle_count_};

    const blitzar_status status = CreateSolver(request, candidate_solver);

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetUnits(blitzar_core::UnitSystem units) noexcept
{
    if (!units.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    const blitzar_physics::GravityParameters candidate_parameters{
        gravity_.gravitational_constant, gravity_.softening, units};

    if (!candidate_parameters.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    SolverVariant candidate_solver(
        std::in_place_type<blitzar_direct::DirectSolver>, candidate_parameters);

    const SolverCreationRequest request{
        solver_kind_, candidate_parameters, barnes_hut_, particle_count_};

    const blitzar_status status = CreateSolver(request, candidate_solver);

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetBarnesHut(blitzar_core::Scalar opening_angle,
    std::size_t max_particles, std::size_t max_cells, std::size_t leaf_capacity,
    std::size_t max_depth) noexcept
{
    return SetBarnesHut(
        {opening_angle, max_particles, max_cells, leaf_capacity, max_depth});
}

blitzar_status Simulation::SetBarnesHut(
    blitzar_barnes_hut::BarnesHutSettings candidate_settings) noexcept
{
    if (!candidate_settings.IsValid() || candidate_settings.max_particles < particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    blitzar_barnes_hut::ThreadWorkspace candidate_workspace(0, 0);

    try {
        candidate_workspace = blitzar_barnes_hut::ThreadWorkspace(
            candidate_settings.max_cells, candidate_settings.max_depth);
    }
    catch (const std::length_error&) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    catch (const std::bad_alloc&) {
        return Remember(BLITZAR_STATUS_ALLOCATION_FAILURE);
    }

    if (solver_kind_ == BLITZAR_SOLVER_BARNES_HUT) {
        SolverVariant candidate_solver(std::in_place_type<blitzar_direct::DirectSolver>, gravity_);
        const SolverCreationRequest request{
            solver_kind_, gravity_, candidate_settings, particle_count_};

        const blitzar_status status = CreateSolver(request, candidate_solver);

        if (status != BLITZAR_STATUS_OK) {
            return Remember(status);
        }

        solver_ = std::move(candidate_solver);
    }

    traversal_workspace_ = std::move(candidate_workspace);
    barnes_hut_ = candidate_settings;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetTimestep(blitzar_core::Scalar timestep) noexcept
{
    if (!std::isfinite(timestep) || timestep <= 0.0) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    timestep_ = timestep;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetSeed(std::uint64_t seed) noexcept
{
    execution_settings_.seed = seed;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetParticles(std::span<const blitzar_core::Scalar> position_x,
    std::span<const blitzar_core::Scalar> position_y,
    std::span<const blitzar_core::Scalar> position_z,
    std::span<const blitzar_core::Scalar> velocity_x,
    std::span<const blitzar_core::Scalar> velocity_y,
    std::span<const blitzar_core::Scalar> velocity_z,
    std::span<const blitzar_core::Scalar> mass) noexcept
{
    return SetParticles({position_x.size(), position_x, position_y, position_z, velocity_x,
        velocity_y, velocity_z, mass, position_x.size()});
}

blitzar_status Simulation::SetParticles(blitzar_core::ParticleStateView input) noexcept
{
    if (!mpi_context_.IsUsable()) {
        return Remember(mpi_context_.Status());
    }

    const bool input_sizes_valid = input.count == particle_count_ && blitzar_core::IsValid(input);
    blitzar_status input_status = SynchronizeSimulationStatus(mpi_context_,
        input_sizes_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "set-particles-input");

    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    SrvParticleInputStage stage;

    input_status = SrvStageParticleInput(input, stage);
    input_status = SynchronizeSimulationStatus(mpi_context_, input_status, "set-particles-stage");

    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    blitzar_parallel::DomainDecomposition candidate_domain;
    blitzar_status domain_status = candidate_domain.Initialize(stage.State(), mpi_context_);

    domain_status =
        SynchronizeSimulationStatus(mpi_context_, domain_status, "set-particles-domain");

    if (domain_status != BLITZAR_STATUS_OK) {
        return Remember(domain_status);
    }

    local_indices_.clear();

    blitzar_status index_status = candidate_domain.LocalIndices(stage.State(), local_indices_);

    index_status = SynchronizeSimulationStatus(mpi_context_, index_status, "set-particles-indices");

    if (index_status != BLITZAR_STATUS_OK) {
        return Remember(index_status);
    }

    const std::size_t local_count = local_indices_.size();
    const blitzar_status capacity_status =
        local_count <= arena_.Count() && local_count <= particle_ids_.size()
            ? BLITZAR_STATUS_OK
            : BLITZAR_STATUS_INVALID_ARGUMENT;

    const blitzar_status synchronized_capacity_status =
        SynchronizeSimulationStatus(mpi_context_, capacity_status, "set-particles-capacity");

    if (synchronized_capacity_status != BLITZAR_STATUS_OK) {
        return Remember(synchronized_capacity_status);
    }

    SrvParticleCommitRequest commit_request{stage, local_indices_, arena_, particles_, accelerations_,
        workspace_, std::span<std::uint64_t>(particle_ids_), domain_, std::move(candidate_domain),
        local_particle_count_, source_particle_count_, exchange_buffer_, particles_ready_};

    const blitzar_status commit_status = SrvCommitStagedParticles(commit_request);

    return Remember(commit_status);
}

blitzar_status Simulation::GetState(std::span<blitzar_core::Scalar> position_x,
    std::span<blitzar_core::Scalar> position_y, std::span<blitzar_core::Scalar> position_z,
    std::span<blitzar_core::Scalar> velocity_x, std::span<blitzar_core::Scalar> velocity_y,
    std::span<blitzar_core::Scalar> velocity_z, std::span<blitzar_core::Scalar> mass) const noexcept
{
    return GetState({position_x.size(), position_x, position_y, position_z, velocity_x,
        velocity_y, velocity_z, mass});
}

blitzar_status Simulation::GetState(blitzar_core::ParticleOutputView output) const noexcept
{
    const bool output_valid = particles_ready_ && output.count >= particle_count_ &&
                              blitzar_core::IsValid(output);

    const blitzar_status output_status = SynchronizeSimulationStatus(mpi_context_,
        output_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "get-state-preflight");

    if (output_status != BLITZAR_STATUS_OK) {
        return Remember(output_status);
    }

    const bool local_state_valid = particles_.Count() == local_particle_count_ &&
                                   local_particle_count_ <= particle_ids_.size() &&
                                   blitzar_core::IsValid(particles_.State());

    const blitzar_status state_status = SynchronizeSimulationStatus(mpi_context_,
        local_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "get-state-state");

    if (state_status != BLITZAR_STATUS_OK) {
        return Remember(state_status);
    }
    if (!mpi_context_.IsDistributed()) {
        const blitzar_core::ParticleStateView state = particles_.State();

        std::copy_n(state.x.begin(), particle_count_, output.x.begin());
        std::copy_n(state.y.begin(), particle_count_, output.y.begin());
        std::copy_n(state.z.begin(), particle_count_, output.z.begin());
        std::copy_n(state.velocity_x.begin(), particle_count_, output.velocity_x.begin());
        std::copy_n(state.velocity_y.begin(), particle_count_, output.velocity_y.begin());
        std::copy_n(state.velocity_z.begin(), particle_count_, output.velocity_z.begin());
        std::copy_n(state.mass.begin(), particle_count_, output.mass.begin());

        return Remember(BLITZAR_STATUS_OK);
    }

    const blitzar_status gather_status = mpi_exchange_.Gather(particles_.State(),
        std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
        gathered_buffer_);

    if (gather_status != BLITZAR_STATUS_OK) {
        return Remember(gather_status);
    }
    if (gathered_buffer_.Size() != particle_count_ || seen_.size() != particle_count_) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }

    std::fill(seen_.begin(), seen_.end(), 0);

    for (const blitzar_parallel::ParticlePacket& packet : gathered_buffer_.View()) {
        if (packet.id >= particle_count_ || seen_[packet.id] != 0 || !std::isfinite(packet.x) ||
            !std::isfinite(packet.y) || !std::isfinite(packet.z) ||
            !std::isfinite(packet.velocity_x) || !std::isfinite(packet.velocity_y) ||
            !std::isfinite(packet.velocity_z) || !std::isfinite(packet.mass) || packet.mass < 0.0) {
            return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
        }

        seen_[packet.id] = 1;
        output.x[packet.id] = packet.x;
        output.y[packet.id] = packet.y;
        output.z[packet.id] = packet.z;
        output.velocity_x[packet.id] = packet.velocity_x;
        output.velocity_y[packet.id] = packet.velocity_y;
        output.velocity_z[packet.id] = packet.velocity_z;
        output.mass[packet.id] = packet.mass;
    }

    if (std::find(seen_.begin(), seen_.end(), 0) != seen_.end()) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }

    return Remember(BLITZAR_STATUS_OK);
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

    blitzar_status status = std::visit(
        [this](auto& solver) {
            if (mpi_context_.IsDistributed()) {
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
                    rollback_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR,
                    "step-state");

                if (state_status != BLITZAR_STATUS_OK) {
                    return state_status;
                }

                SrvTransactionState transaction_state{
                    arena_, particles_, accelerations_, workspace_,
                    std::span<std::uint64_t>(particle_ids_), local_particle_count_,
                    source_particle_count_, exchange_buffer_, rollback_arena_buffer_,
                    rollback_force_buffer_, rollback_exchange_buffer_};

                SrvStepTransaction transaction(transaction_state);
                blitzar_status prepare_status = transaction.Prepare();

                prepare_status =
                    SynchronizeSimulationStatus(mpi_context_, prepare_status, "step-prepare");

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
                    [this, rollback_particle_count](
                        blitzar_particles::ParticleBuffer& current_particles,
                        blitzar_particles::AccelerationBuffer& current_accelerations,
                        blitzar_integration::LeapfrogWorkspace& current_workspace)
                    -> blitzar_integration_kdk::DriftTransition {
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
                        migration_buffer_, arena_, current_particles, current_accelerations,
                        current_workspace, std::span<std::uint64_t>(particle_ids_), particle_count_,
                        local_particle_count_};

                    migration_status = SrvStoreLocalPackets(migration_request);
                    migration_status = SynchronizeSimulationStatus(
                        mpi_context_, migration_status, "migrate-commit");

                    if (migration_status != BLITZAR_STATUS_OK) {
                        return {migration_status, false};
                    }

                    source_particle_count_ = local_particle_count_;

                    return {BLITZAR_STATUS_OK, true};
                };

                blitzar_integration_kdk::AdvanceState<Dispatcher,
                    blitzar_barnes_hut::ThreadWorkspace>
                    advance_state{particles_, accelerations_, workspace_, dispatcher, timestep_,
                        execution_settings_, traversal_workspace_, particles_.State()};

                blitzar_integration_kdk::AdvanceHooks advance_hooks{
                    migrate_after_drift, rollback};

                blitzar_integration_kdk::AdvanceRequest advance_request{
                    advance_state, advance_hooks};

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

            using SolverType = std::remove_reference_t<decltype(solver)>;
            using Dispatcher = SrvSolverDispatcher<SolverType>;
            Dispatcher dispatcher(
                SrvSolverDispatchContext<SolverType>{
                    hip_context_, solver, gravity_, barnes_hut_, last_backend_});

            blitzar_integration_kdk::AdvanceState<Dispatcher,
                blitzar_barnes_hut::ThreadWorkspace>
                advance_state{particles_, accelerations_, workspace_, dispatcher, timestep_,
                    execution_settings_, traversal_workspace_, particles_.State()};

            return integrator_.Advance(advance_state);
        },
        solver_);

    return Remember(status);
}

} // namespace blitzar_sdk
