#include "simulation/Sim.hpp"

#include <new>
#include <stdexcept>

namespace blitzar_sim {

Sim::Sim(std::size_t particle_count)
    : particle_count_(particle_count), runtime_(particle_count), particle_state_(particle_count),
      particle_source_(particle_count), gravity_{},
      barnes_hut_{
          0.5, particle_count == 0 ? 1 : particle_count, DefaultMaxCells(particle_count), 8, 32},
      solver_kind_(BLITZAR_SOLVER_DIRECT), integrator_kind_(BLITZAR_INTEGRATOR_LEAPFROG_KDK),
      timestep_(1.0), particles_ready_(false), execution_settings_{}, snapshot_header_{},
      last_status_(runtime_.Mpi().Status()), last_backend_(BLITZAR_BACKEND_CPU),
      solver_(std::in_place_type<DirectSolverBundle>, gravity_, particle_count), integrator_{},
      particle_ids_(particle_count), output_order_(particle_count), local_particle_count_(0),
      exchange_buffer_{}, rollback_arena_buffer_{}, rollback_force_buffer_{},
      rollback_exchange_buffer_{}, migration_buffer_{}, gathered_buffer_{}
{
    const blitzar_status capacity_status = runtime_.Exchange().CapacityStatus();

    if (capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE) {
        throw std::bad_alloc();
    }

    if (capacity_status != BLITZAR_STATUS_OK) {
        throw std::length_error("simulation capacity preparation failed");
    }

    if (particle_state_.Particles().SetCount(0) != BLITZAR_STATUS_OK ||
        particle_state_.Accelerations().SetCount(0) != BLITZAR_STATUS_OK ||
        particle_state_.Checkpoint().SetCount(0) != BLITZAR_STATUS_OK) {
        throw std::length_error("simulation local capacity initialization failed");
    }

    rollback_arena_buffer_.Reserve(particle_count_);
    rollback_force_buffer_.Reserve(particle_count_);
    exchange_buffer_.Reserve(particle_count_);
    rollback_exchange_buffer_.Reserve(particle_count_);
    migration_buffer_.Reserve(particle_count_);
    gathered_buffer_.Reserve(particle_count_);
    snapshot_header_.particle_count = particle_count_;
}

std::size_t Sim::LocalCapacity(std::size_t particle_count, int rank_count) noexcept
{
    if (rank_count <= 1 || particle_count == 0) {
        return particle_count;
    }

    const std::size_t peers = static_cast<std::size_t>(rank_count);

    return particle_count / peers + (particle_count % peers == 0 ? 0 : 1);
}

std::size_t Sim::RemoteCapacity(std::size_t particle_count, int rank_count) noexcept
{
    if (rank_count <= 1 || particle_count == 0) {
        return 0;
    }

    const std::size_t peers = static_cast<std::size_t>(rank_count);
    const std::size_t minimum_local_capacity = particle_count / peers;

    return particle_count - minimum_local_capacity;
}

blitzar_status Sim::EnsureLocalCapacity(std::size_t capacity) noexcept
{
    if (capacity > particle_count_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_status status = particle_state_.Arena().Reserve(capacity);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    try {
        if (particle_ids_.size() < capacity) {
            particle_ids_.resize(capacity);
        }
        if (output_order_.size() < capacity) {
            output_order_.resize(capacity);
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    if (!rollback_arena_buffer_.EnsureCapacity(capacity) ||
        !rollback_force_buffer_.EnsureCapacity(capacity)) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Sim::LastStatus() const noexcept
{
    return last_status_.load(std::memory_order_relaxed);
}

blitzar_backend_kind Sim::LastBackend() const noexcept
{
    return last_backend_.load(std::memory_order_relaxed);
}

void Sim::SetFaultForTesting(blitzar_hip::Fault fault) noexcept
{
    runtime_.Accelerator().SetFaultForTesting(fault);
}

void Sim::SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode mode) noexcept
{
    overlap_mode_ = mode;

    overlap_trace_.Reset(mode);
}

const blitzar_parallel::MpiOverlapTrace& Sim::LastMpiOverlapTrace() const noexcept
{
    return overlap_trace_;
}

std::size_t Sim::ParticleCount() const noexcept
{
    return particle_count_;
}

int Sim::MpiRank() const noexcept
{
    return runtime_.Mpi().Rank();
}

int Sim::MpiSize() const noexcept
{
    return runtime_.Mpi().Size();
}

std::size_t Sim::LocalParticleCount() const noexcept
{
    return local_particle_count_;
}

bool Sim::IsSnapshotBoundaryReady() const noexcept
{
    return !runtime_.Mpi().IsGhostExchangeActive(runtime_.Exchange().PersistentGhostExchange());
}

const blitzar_parallel::MpiMigrationTrace& Sim::LastMpiMigrationTrace() const noexcept
{
    return migration_trace_;
}

blitzar_status Sim::Remember(blitzar_status status) const noexcept
{
    last_status_.store(status, std::memory_order_relaxed);

    return status;
}

} // namespace blitzar_sim
