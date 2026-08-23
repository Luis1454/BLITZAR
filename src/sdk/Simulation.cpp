#include "sdk/Simulation.hpp"

#include <new>
#include <stdexcept>

namespace blitzar_sdk {

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

blitzar_status Simulation::Remember(blitzar_status status) const noexcept
{
    last_status_.store(status, std::memory_order_relaxed);

    return status;
}

} // namespace blitzar_sdk
