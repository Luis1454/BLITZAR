#ifndef BLITZAR_SIMULATION_SIM_HPP
#define BLITZAR_SIMULATION_SIM_HPP

#include "core/CoreExecution.hpp"
#include "core/CoreSnapshot.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"
#include "mpi/domain/MpiDomainDecomposition.hpp"
#include "mpi/exchange/MpiExchangeTrace.hpp"
#include "particles/source/ParticleSourceBuffer.hpp"
#include "simulation/runtime/SimRuntime.hpp"
#include "simulation/solver/SimSolverVariant.hpp"
#include "simulation/state/SimParticleState.hpp"

#include <atomic>
#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_sim {

struct SimParticleStage;
class SimTransaction;

class Sim final {
public:
    explicit Sim(std::size_t particle_count);

    [[nodiscard]] blitzar_status LastStatus() const noexcept;
    [[nodiscard]] blitzar_backend_kind LastBackend() const noexcept;
    [[nodiscard]] std::size_t ParticleCount() const noexcept;
    [[nodiscard]] int MpiRank() const noexcept;
    [[nodiscard]] int MpiSize() const noexcept;
    [[nodiscard]] std::size_t LocalParticleCount() const noexcept;

    [[nodiscard]] blitzar_status SetSolver(blitzar_solver_kind solver) noexcept;
    [[nodiscard]] blitzar_status SetIntegrator(blitzar_integrator_kind integrator) noexcept;
    [[nodiscard]] blitzar_status SetGravity(
        blitzar_core::Scalar gravitational_constant, blitzar_core::Scalar softening) noexcept;
    [[nodiscard]] blitzar_status SetUnits(blitzar_core::UnitSystem units) noexcept;
    [[nodiscard]] blitzar_status SetBarnesHut(
        blitzar_barnes_hut::BarnesHutSettings settings) noexcept;
    [[nodiscard]] blitzar_status SetTimestep(blitzar_core::Scalar timestep) noexcept;
    [[nodiscard]] blitzar_status SetSeed(std::uint64_t seed) noexcept;
    [[nodiscard]] blitzar_status SetParticles(blitzar_core::ParticleStateView input) noexcept;
    [[nodiscard]] blitzar_status GetState(blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status GetLocalState(blitzar_core::ParticleOutputView output,
        std::span<std::uint64_t> ids, std::size_t& count) const noexcept;
    [[nodiscard]] blitzar_status Step() noexcept;
    void SetFaultForTesting(blitzar_hip::Fault fault) noexcept;
    void SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode mode) noexcept;
    [[nodiscard]] const blitzar_parallel::MpiOverlapTrace& LastMpiOverlapTrace() const noexcept;
    [[nodiscard]] const blitzar_parallel::MpiMigrationTrace& LastMpiMigrationTrace() const noexcept;

private:
    struct SolverCreationRequest;

    [[nodiscard]] static std::size_t LocalCapacity(
        std::size_t particle_count, int rank_count) noexcept;
    [[nodiscard]] static std::size_t RemoteCapacity(
        std::size_t particle_count, int rank_count) noexcept;
    [[nodiscard]] static std::size_t DefaultMaxCells(std::size_t particle_count) noexcept;
    [[nodiscard]] static blitzar_status CreateSolver(
        const SolverCreationRequest& request, SolverVariant& solver) noexcept;
    [[nodiscard]] blitzar_status RebuildSolver(const blitzar_physics::GravityParameters& gravity,
        const blitzar_barnes_hut::BarnesHutSettings& barnes_hut, SolverVariant& solver) noexcept;
    [[nodiscard]] blitzar_status EnsureLocalCapacity(std::size_t capacity) noexcept;
    [[nodiscard]] blitzar_status Remember(blitzar_status status) const noexcept;
    [[nodiscard]] bool ValidateParticleInput(blitzar_core::ParticleStateView input) const noexcept;
    [[nodiscard]] blitzar_status DistributeParticles(SimParticleStage& stage,
        blitzar_parallel::MpiDomainDecomposition& domain,
        blitzar_parallel::PacketBuffer& distributed) noexcept;
    [[nodiscard]] bool ValidateOutput(blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status CopyLocalState(
        blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status GatherState(
        blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status PrepareDistributedStep(
        std::size_t rollback_particle_count, SimTransaction& transaction) noexcept;

    template <typename Solver> [[nodiscard]] blitzar_status StepWithSolver(Solver& solver) noexcept;

    template <typename Solver>
    [[nodiscard]] blitzar_status StepDistributed(Solver& solver) noexcept;

    template <typename Solver> [[nodiscard]] blitzar_status StepLocal(Solver& solver) noexcept;

    [[nodiscard]] blitzar_integration_kdk::DriftTransition MigrateAfterDrift(
        std::size_t rollback_particle_count, blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::ParticleAccelerationBuffer& accelerations,
        blitzar_integration::KdkCheckpoint& checkpoint) noexcept;

    std::size_t particle_count_;
    SimRuntime runtime_;
    SimParticleState particle_state_;
    blitzar_particles::ParticleSourceBuffer particle_source_;
    blitzar_physics::GravityParameters gravity_;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut_;
    blitzar_solver_kind solver_kind_;
    blitzar_integrator_kind integrator_kind_;
    blitzar_core::Scalar timestep_;
    bool particles_ready_;
    blitzar_core::ExecutionSettings execution_settings_;
    blitzar_core::SnapshotHeader snapshot_header_;
    blitzar_parallel::MpiOverlapMode overlap_mode_{blitzar_parallel::MpiOverlapMode::Overlapped};
    blitzar_parallel::MpiOverlapTrace overlap_trace_{};
    blitzar_parallel::MpiMigrationTrace migration_trace_{};
    mutable std::atomic<blitzar_status> last_status_;
    mutable std::atomic<blitzar_backend_kind> last_backend_;
    SolverVariant solver_;
    blitzar_integration::KdkLeapfrog integrator_;
    std::vector<std::uint64_t> particle_ids_;
    mutable std::vector<std::size_t> output_order_;
    std::size_t local_particle_count_;
    blitzar_parallel::PacketBuffer exchange_buffer_;
    blitzar_parallel::PacketBuffer rollback_arena_buffer_;
    blitzar_parallel::PacketBuffer rollback_force_buffer_;
    blitzar_parallel::PacketBuffer rollback_exchange_buffer_;
    blitzar_parallel::PacketBuffer migration_buffer_;
    mutable blitzar_parallel::PacketBuffer gathered_buffer_;
};

} // namespace blitzar_sim

#endif
