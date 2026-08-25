#ifndef BLITZAR_SDK_SIMULATION_HPP
#define BLITZAR_SDK_SIMULATION_HPP

#include "core/Execution.hpp"
#include "core/Snapshot.hpp"
#include "core/Solver.hpp"
#include "gpu/HipContext.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiExchange.hpp"
#include "parallel/MpiTrace.hpp"
#include "parallel/MpiTypes.hpp"
#include "particles/ParticleBuffer.hpp"
#include "particles/SourceBuffer.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/barnes_hut/ThreadStackPool.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"

#include <atomic>
#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace blitzar_sdk {

struct ParticleInputStage;
class StepTransaction;

class Simulation final {
public:
    explicit Simulation(std::size_t particle_count);

    [[nodiscard]] blitzar_status LastStatus() const noexcept;
    [[nodiscard]] blitzar_backend_kind LastBackend() const noexcept;
    [[nodiscard]] std::size_t ParticleCount() const noexcept;

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
    [[nodiscard]] blitzar_status Step() noexcept;
    void SetHipFaultForTesting(blitzar_gpu::HipFault fault) noexcept;
    void SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode mode) noexcept;
    [[nodiscard]] const blitzar_parallel::MpiOverlapTrace& LastMpiOverlapTrace() const noexcept;

private:
    using SolverVariant =
        std::variant<blitzar_direct::DirectSolver, blitzar_barnes_hut::BarnesHutSolver,
            blitzar_fmm::FmmSolver>;

    struct SolverCreationRequest;

    [[nodiscard]] static std::size_t LocalCapacity(
        std::size_t particle_count, int rank_count) noexcept;
    [[nodiscard]] static std::size_t RemoteCapacity(
        std::size_t particle_count, int rank_count) noexcept;
    [[nodiscard]] static std::size_t DefaultMaxCells(std::size_t particle_count) noexcept;
    [[nodiscard]] static blitzar_status CreateSolver(const SolverCreationRequest& request,
        SolverVariant& solver) noexcept;
    [[nodiscard]] blitzar_status RebuildSolver(
        const blitzar_physics::GravityParameters& gravity,
        const blitzar_barnes_hut::BarnesHutSettings& barnes_hut,
        SolverVariant& solver) noexcept;
    [[nodiscard]] blitzar_status EnsureLocalCapacity(std::size_t capacity) noexcept;
    [[nodiscard]] blitzar_status Remember(blitzar_status status) const noexcept;
    [[nodiscard]] bool ValidateParticleInput(blitzar_core::ParticleStateView input) const noexcept;
    [[nodiscard]] blitzar_status DistributeParticles(ParticleInputStage& stage,
        blitzar_parallel::DomainDecomposition& domain,
        blitzar_parallel::PacketBuffer& distributed) noexcept;
    [[nodiscard]] bool ValidateOutput(blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status CopyLocalState(blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status GatherState(blitzar_core::ParticleOutputView output) const noexcept;
    [[nodiscard]] blitzar_status PrepareDistributedStep(
        std::size_t rollback_particle_count, StepTransaction& transaction) noexcept;

    template <typename Solver>
    [[nodiscard]] blitzar_status StepWithSolver(Solver& solver) noexcept;

    template <typename Solver>
    [[nodiscard]] blitzar_status StepDistributed(Solver& solver) noexcept;

    template <typename Solver>
    [[nodiscard]] blitzar_status StepLocal(Solver& solver) noexcept;

    [[nodiscard]] blitzar_integration_kdk::DriftTransition MigrateAfterDrift(
        std::size_t rollback_particle_count,
        blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations,
        blitzar_integration::KdkCheckpoint& checkpoint) noexcept;

    std::size_t particle_count_;
    blitzar_parallel::MpiContext mpi_context_;
    blitzar_parallel::DomainDecomposition domain_;
    blitzar_parallel::MpiExchange mpi_exchange_;
    blitzar_gpu::HipContext hip_context_;
    blitzar_particles::ParticleArena arena_;
    blitzar_particles::ParticleBuffer particles_;
    blitzar_particles::AccelerationBuffer accelerations_;
    blitzar_integration::KdkCheckpoint checkpoint_;
    blitzar_particles::SourceBuffer source_;
    blitzar_physics::GravityParameters gravity_;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut_;
    blitzar_barnes_hut::ThreadStackPool traversal_stacks_;
    blitzar_solver_kind solver_kind_;
    blitzar_integrator_kind integrator_kind_;
    blitzar_core::Scalar timestep_;
    bool particles_ready_;
    blitzar_core::ExecutionSettings execution_settings_;
    blitzar_core::SnapshotHeader snapshot_header_;
    blitzar_parallel::MpiOverlapMode overlap_mode_{
        blitzar_parallel::MpiOverlapMode::Overlapped};
    blitzar_parallel::MpiOverlapTrace overlap_trace_{};
    mutable std::atomic<blitzar_status> last_status_;
    mutable std::atomic<blitzar_backend_kind> last_backend_;
    SolverVariant solver_;
    blitzar_integration::LeapfrogKdk integrator_;
    std::vector<std::uint64_t> particle_ids_;
    std::size_t local_particle_count_;
    blitzar_parallel::PacketBuffer exchange_buffer_;
    blitzar_parallel::PacketBuffer rollback_arena_buffer_;
    blitzar_parallel::PacketBuffer rollback_force_buffer_;
    blitzar_parallel::PacketBuffer rollback_exchange_buffer_;
    blitzar_parallel::PacketBuffer migration_buffer_;
    mutable blitzar_parallel::PacketBuffer gathered_buffer_;
};

} // namespace blitzar_sdk

#endif
