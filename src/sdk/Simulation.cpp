#include "sdk/Simulation.hpp"

#include <new>
#include <stdexcept>

namespace blitzar_sdk {

Simulation::Simulation(std::size_t particle_count)
    : particle_count_(particle_count), mpi_context_(), domain_(),
      mpi_exchange_(mpi_context_, domain_, particle_count, particle_count), hip_context_(),
      arena_(particle_count),
      particles_(arena_), accelerations_(arena_), checkpoint_(arena_),
      source_(particle_count),
      gravity_{},
      barnes_hut_{
          0.5, particle_count == 0 ? 1 : particle_count, DefaultMaxCells(particle_count), 8, 32},
      traversal_stacks_(barnes_hut_.max_cells, barnes_hut_.max_depth),
      solver_kind_(BLITZAR_SOLVER_DIRECT), integrator_kind_(BLITZAR_INTEGRATOR_LEAPFROG_KDK),
      timestep_(1.0), particles_ready_(false), execution_settings_{}, snapshot_header_{},
      last_status_(mpi_context_.Status()), last_backend_(BLITZAR_BACKEND_CPU),
      solver_(std::in_place_type<blitzar_direct::DirectSolver>, gravity_, particle_count),
      integrator_{}, particle_ids_(particle_count),
      local_particle_count_(0), exchange_buffer_{}, rollback_arena_buffer_{},
      rollback_force_buffer_{}, rollback_exchange_buffer_{}, migration_buffer_{},
      gathered_buffer_{}
{
    const blitzar_status capacity_status = mpi_exchange_.CapacityStatus();

    if (capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE) {
        throw std::bad_alloc();
    }

    if (capacity_status != BLITZAR_STATUS_OK) {
        throw std::length_error("simulation capacity preparation failed");
    }

    if (particles_.SetCount(0) != BLITZAR_STATUS_OK ||
        accelerations_.SetCount(0) != BLITZAR_STATUS_OK ||
        checkpoint_.SetCount(0) != BLITZAR_STATUS_OK) {
        throw std::length_error("simulation local capacity initialization failed");
    }

    rollback_arena_buffer_.Reserve(particle_count_);
    rollback_force_buffer_.Reserve(particle_count_);
    exchange_buffer_.Reserve(particle_count_);
    rollback_exchange_buffer_.Reserve(particle_count_);
    migration_buffer_.Reserve(particle_count_);
    snapshot_header_.particle_count = particle_count_;
}

std::size_t Simulation::LocalCapacity(std::size_t particle_count, int rank_count) noexcept
{
    if (rank_count <= 1 || particle_count == 0) {
        return particle_count;
    }

    const std::size_t peers = static_cast<std::size_t>(rank_count);

    return particle_count / peers + (particle_count % peers == 0 ? 0 : 1);
}

std::size_t Simulation::RemoteCapacity(std::size_t particle_count, int rank_count) noexcept
{
    if (rank_count <= 1 || particle_count == 0) {
        return 0;
    }

    const std::size_t peers = static_cast<std::size_t>(rank_count);
    const std::size_t minimum_local_capacity = particle_count / peers;

    return particle_count - minimum_local_capacity;
}

blitzar_status Simulation::EnsureLocalCapacity(std::size_t capacity) noexcept
{
    if (capacity > particle_count_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_status status = arena_.Reserve(capacity);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    try {
        if (particle_ids_.size() < capacity) {
            particle_ids_.resize(capacity);
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

void Simulation::SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode mode) noexcept
{
    overlap_mode_ = mode;

    overlap_trace_.Reset(mode);
}

const blitzar_parallel::MpiOverlapTrace& Simulation::LastMpiOverlapTrace() const noexcept
{
    return overlap_trace_;
}

std::size_t Simulation::ParticleCount() const noexcept
{
    return particle_count_;
}

blitzar_status Simulation::Remember(blitzar_status status) const noexcept
{
    last_status_.store(status, std::memory_order_relaxed);

    return status;
}

} // namespace blitzar_sdk
