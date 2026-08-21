#include "sdk/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_sdk {

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
    : particle_count_(particle_count),
      arena_(std::make_shared<blitzar_particles::ParticleArena>(particle_count)),
      particles_(arena_),
      accelerations_(arena_),
      workspace_(arena_),
      gravity_{},
      barnes_hut_{
          0.5,
          particle_count == 0 ? 1 : particle_count,
          DefaultMaxCells(particle_count),
          8,
          32},
      traversal_storage_(barnes_hut_.max_cells),
      solver_kind_(BLITZAR_SOLVER_DIRECT),
      integrator_kind_(BLITZAR_INTEGRATOR_LEAPFROG_KDK),
      timestep_(1.0),
      particles_ready_(false),
      execution_settings_{},
      snapshot_header_{},
      last_status_(BLITZAR_STATUS_OK),
      solver_(std::in_place_type<blitzar_direct::DirectSolver>, gravity_),
      integrator_{}
{
    snapshot_header_.particle_count = particle_count_;
}

blitzar_status Simulation::LastStatus() const noexcept
{
    return last_status_.load(std::memory_order_relaxed);
}

std::size_t Simulation::ParticleCount() const noexcept
{
    return particle_count_;
}

blitzar_status Simulation::CreateSolver(
    blitzar_solver_kind solver_kind,
    blitzar_physics::GravityParameters gravity,
    blitzar_barnes_hut::BarnesHutSettings barnes_hut,
    SolverVariant& solver) noexcept
{
    try {
        switch (solver_kind) {
        case BLITZAR_SOLVER_DIRECT:
            solver.emplace<blitzar_direct::DirectSolver>(gravity);
            return BLITZAR_STATUS_OK;
        case BLITZAR_SOLVER_BARNES_HUT:
            if (!barnes_hut.IsValid()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            solver.emplace<blitzar_barnes_hut::BarnesHutSolver>(
                gravity, barnes_hut);
            return BLITZAR_STATUS_OK;
        case BLITZAR_SOLVER_FMM:
        case BLITZAR_SOLVER_PM:
        case BLITZAR_SOLVER_TREEPM:
            return BLITZAR_STATUS_UNSUPPORTED;
        default:
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
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
    const blitzar_status status =
        CreateSolver(solver, gravity_, barnes_hut_, candidate);
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
    blitzar_core::Scalar gravitational_constant,
    blitzar_core::Scalar softening) noexcept
{
    const blitzar_physics::GravityParameters candidate_parameters{
        gravitational_constant, softening, gravity_.units};
    if (!candidate_parameters.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    SolverVariant candidate_solver(
        std::in_place_type<blitzar_direct::DirectSolver>, candidate_parameters);
    const blitzar_status status = CreateSolver(
        solver_kind_, candidate_parameters, barnes_hut_, candidate_solver);
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
    const blitzar_status status = CreateSolver(
        solver_kind_, candidate_parameters, barnes_hut_, candidate_solver);
    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }
    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::SetBarnesHut(
    blitzar_core::Scalar opening_angle,
    std::size_t max_particles,
    std::size_t max_cells,
    std::size_t leaf_capacity,
    std::size_t max_depth) noexcept
{
    const blitzar_barnes_hut::BarnesHutSettings candidate_settings{
        opening_angle, max_particles, max_cells, leaf_capacity, max_depth};
    if (!candidate_settings.IsValid() || max_particles < particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    std::vector<std::size_t> candidate_workspace;
    try {
        candidate_workspace.resize(candidate_settings.max_cells);
    } catch (const std::length_error&) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    } catch (const std::bad_alloc&) {
        return Remember(BLITZAR_STATUS_ALLOCATION_FAILURE);
    }
    if (solver_kind_ == BLITZAR_SOLVER_BARNES_HUT) {
        SolverVariant candidate_solver(
            std::in_place_type<blitzar_direct::DirectSolver>, gravity_);
        const blitzar_status status = CreateSolver(
            solver_kind_, gravity_, candidate_settings, candidate_solver);
        if (status != BLITZAR_STATUS_OK) {
            return Remember(status);
        }
        solver_ = std::move(candidate_solver);
    }
    traversal_storage_ = std::move(candidate_workspace);
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

blitzar_status Simulation::SetParticles(
    std::span<const blitzar_core::Scalar> position_x,
    std::span<const blitzar_core::Scalar> position_y,
    std::span<const blitzar_core::Scalar> position_z,
    std::span<const blitzar_core::Scalar> velocity_x,
    std::span<const blitzar_core::Scalar> velocity_y,
    std::span<const blitzar_core::Scalar> velocity_z,
    std::span<const blitzar_core::Scalar> mass) noexcept
{
    if (position_x.size() != particle_count_ ||
        position_y.size() != particle_count_ ||
        position_z.size() != particle_count_ ||
        velocity_x.size() != particle_count_ ||
        velocity_y.size() != particle_count_ ||
        velocity_z.size() != particle_count_ || mass.size() != particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    for (std::size_t index = 0; index < particle_count_; ++index) {
        if (!std::isfinite(position_x[index]) ||
            !std::isfinite(position_y[index]) ||
            !std::isfinite(position_z[index]) ||
            !std::isfinite(velocity_x[index]) ||
            !std::isfinite(velocity_y[index]) ||
            !std::isfinite(velocity_z[index]) || !std::isfinite(mass[index]) ||
            mass[index] < 0.0) {
            return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
        }
    }
    for (std::size_t index = 0; index < particle_count_; ++index) {
        if (particles_.SetPosition(
                index, {position_x[index], position_y[index], position_z[index]}) !=
                BLITZAR_STATUS_OK ||
            particles_.SetVelocity(
                index,
                {velocity_x[index], velocity_y[index], velocity_z[index]}) !=
                BLITZAR_STATUS_OK ||
            particles_.SetMass(index, mass[index]) != BLITZAR_STATUS_OK) {
            return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
        }
    }
    particles_ready_ = true;
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::GetState(
    std::span<blitzar_core::Scalar> position_x,
    std::span<blitzar_core::Scalar> position_y,
    std::span<blitzar_core::Scalar> position_z,
    std::span<blitzar_core::Scalar> velocity_x,
    std::span<blitzar_core::Scalar> velocity_y,
    std::span<blitzar_core::Scalar> velocity_z,
    std::span<blitzar_core::Scalar> mass) const noexcept
{
    if (!particles_ready_ || position_x.size() < particle_count_ ||
        position_y.size() < particle_count_ ||
        position_z.size() < particle_count_ ||
        velocity_x.size() < particle_count_ ||
        velocity_y.size() < particle_count_ ||
        velocity_z.size() < particle_count_ || mass.size() < particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    const blitzar_core::ParticleStateView state = particles_.State();
    std::copy_n(state.x.begin(), particle_count_, position_x.begin());
    std::copy_n(state.y.begin(), particle_count_, position_y.begin());
    std::copy_n(state.z.begin(), particle_count_, position_z.begin());
    std::copy_n(state.velocity_x.begin(), particle_count_, velocity_x.begin());
    std::copy_n(state.velocity_y.begin(), particle_count_, velocity_y.begin());
    std::copy_n(state.velocity_z.begin(), particle_count_, velocity_z.begin());
    std::copy_n(state.mass.begin(), particle_count_, mass.begin());
    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::Step() noexcept
{
    if (!particles_ready_ || integrator_kind_ != BLITZAR_INTEGRATOR_LEAPFROG_KDK ||
        !std::isfinite(timestep_) || timestep_ <= 0.0) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    const blitzar_status status = std::visit(
        [this](auto& solver) {
            return integrator_.Advance(
                particles_,
                accelerations_,
                workspace_,
                solver,
                timestep_,
                execution_settings_,
                std::span<std::size_t>(traversal_storage_));
        },
        solver_);
    return Remember(status);
}

}  // namespace blitzar_sdk
