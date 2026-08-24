#include "sdk/Simulation.hpp"

#include <new>
#include <stdexcept>

namespace blitzar_sdk {

Simulation::Simulation(std::size_t particle_count)
    : particle_count_(particle_count), mpi_context_(), domain_(),
      mpi_exchange_(mpi_context_, domain_, particle_count,
          particle_count),
      hip_context_(), arena_(particle_count),
      particles_(arena_), accelerations_(arena_), checkpoint_(arena_), source_(particle_count_),
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
      gathered_buffer_{}, seen_(particle_count, 0)
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
    gathered_buffer_.Reserve(particle_count_);
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

blitzar_status Simulation::Remember(blitzar_status status) const noexcept
{
    last_status_.store(status, std::memory_order_relaxed);

    return status;
}

} // namespace blitzar_sdk
