#include "simulation/Sim.hpp"

#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_sim {

struct Sim::SolverCreationRequest final {
    blitzar_solver_kind solver_kind{BLITZAR_SOLVER_DIRECT};
    blitzar_physics::GravityParameters gravity{};
    blitzar_barnes_hut::BarnesHutSettings barnes_hut{};
    std::size_t staging_capacity{};
};

std::size_t Sim::DefaultMaxCells(std::size_t particle_count) noexcept
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

blitzar_status Sim::CreateSolver(
    const SolverCreationRequest& request, SolverVariant& solver) noexcept
{
    try {
        switch (request.solver_kind) {
        case BLITZAR_SOLVER_DIRECT:

            solver.emplace<DirectSolverBundle>(request.gravity, request.staging_capacity);

            return BLITZAR_STATUS_OK;

        case BLITZAR_SOLVER_BARNES_HUT:

            if (!request.barnes_hut.IsValid()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            solver.emplace<BarnesHutSolverBundle>(
                request.gravity, request.barnes_hut, request.staging_capacity);

            return BLITZAR_STATUS_OK;

        case BLITZAR_SOLVER_FMM:

            if (!request.barnes_hut.IsValid()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            solver.emplace<FmmSolverBundle>(
                request.gravity, request.barnes_hut, request.staging_capacity);

            return BLITZAR_STATUS_OK;

        case BLITZAR_SOLVER_PM:

            solver.emplace<PmSolverBundle>(request.gravity, request.staging_capacity);

            return BLITZAR_STATUS_OK;

        case BLITZAR_SOLVER_TREEPM:

            if (!request.barnes_hut.IsValid()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            solver.emplace<TreePmSolverBundle>(
                request.gravity, request.barnes_hut, request.staging_capacity);

            return BLITZAR_STATUS_OK;

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

blitzar_status Sim::RebuildSolver(const blitzar_physics::GravityParameters& gravity,
    const blitzar_barnes_hut::BarnesHutSettings& barnes_hut, SolverVariant& solver) noexcept
{
    const SolverCreationRequest request{solver_kind_, gravity, barnes_hut, particle_count_};

    return CreateSolver(request, solver);
}

blitzar_status Sim::SetSolver(blitzar_solver_kind solver) noexcept
{
    SolverVariant candidate(std::in_place_type<DirectSolverBundle>, gravity_, particle_count_);
    const SolverCreationRequest request{solver, gravity_, barnes_hut_, particle_count_};

    const blitzar_status status = CreateSolver(request, candidate);

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    solver_kind_ = solver;
    solver_ = std::move(candidate);

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Sim::SetIntegrator(blitzar_integrator_kind integrator) noexcept
{
    if (integrator != BLITZAR_INTEGRATOR_LEAPFROG_KDK) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    integrator_kind_ = integrator;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Sim::SetGravity(
    blitzar_core::Scalar gravitational_constant, blitzar_core::Scalar softening) noexcept
{
    const blitzar_physics::GravityParameters candidate_parameters{
        gravitational_constant, softening, gravity_.units};

    if (!candidate_parameters.IsValid()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    SolverVariant candidate_solver(
        std::in_place_type<DirectSolverBundle>, candidate_parameters, particle_count_);

    const blitzar_status status =
        RebuildSolver(candidate_parameters, barnes_hut_, candidate_solver);

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Sim::SetUnits(blitzar_core::UnitSystem units) noexcept
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
        std::in_place_type<DirectSolverBundle>, candidate_parameters, particle_count_);

    const blitzar_status status =
        RebuildSolver(candidate_parameters, barnes_hut_, candidate_solver);

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    gravity_ = candidate_parameters;
    solver_ = std::move(candidate_solver);

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Sim::SetBarnesHut(blitzar_barnes_hut::BarnesHutSettings candidate_settings) noexcept
{
    if (!candidate_settings.IsValid() || candidate_settings.max_particles < particle_count_) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    if (solver_kind_ == BLITZAR_SOLVER_BARNES_HUT || solver_kind_ == BLITZAR_SOLVER_FMM ||
        solver_kind_ == BLITZAR_SOLVER_TREEPM) {
        SolverVariant candidate_solver(
            std::in_place_type<DirectSolverBundle>, gravity_, particle_count_);

        const blitzar_status status = RebuildSolver(gravity_, candidate_settings, candidate_solver);

        if (status != BLITZAR_STATUS_OK) {
            return Remember(status);
        }

        solver_ = std::move(candidate_solver);
    }

    barnes_hut_ = candidate_settings;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Sim::SetTimestep(blitzar_core::Scalar timestep) noexcept
{
    if (!std::isfinite(timestep) || timestep <= 0.0) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    timestep_ = timestep;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Sim::SetSeed(std::uint64_t seed) noexcept
{
    execution_settings_.seed = seed;

    return Remember(BLITZAR_STATUS_OK);
}

} // namespace blitzar_sim
