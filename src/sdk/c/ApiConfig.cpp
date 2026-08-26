#include "sdk/c/ApiState.hpp"

extern "C" blitzar_status blitzar_simulation_set_solver(
    blitzar_simulation* simulation, blitzar_solver_kind solver)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetSolver(solver);
}

extern "C" blitzar_status blitzar_simulation_set_integrator(
    blitzar_simulation* simulation, blitzar_integrator_kind integrator)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetIntegrator(integrator);
}

extern "C" blitzar_status blitzar_simulation_set_gravity(
    blitzar_simulation* simulation, double gravitational_constant, double softening)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetGravity(gravitational_constant, softening);
}

extern "C" blitzar_status blitzar_simulation_set_units(
    blitzar_simulation* simulation, double length_scale, double mass_scale, double time_scale)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetUnits({length_scale, mass_scale, time_scale});
}

extern "C" blitzar_status blitzar_simulation_set_barnes_hut(blitzar_simulation* simulation,
    double opening_angle, std::int64_t max_particles, std::int64_t max_cells,
    std::int64_t leaf_capacity, std::int64_t max_depth)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t converted_max_particles = 0;
    std::size_t converted_max_cells = 0;
    std::size_t converted_leaf_capacity = 0;
    std::size_t converted_max_depth = 0;

    if (!blitzar_sdk_api::TryConvertCount(max_particles, converted_max_particles) ||
        !blitzar_sdk_api::TryConvertCount(max_cells, converted_max_cells) ||
        !blitzar_sdk_api::TryConvertCount(leaf_capacity, converted_leaf_capacity) ||
        !blitzar_sdk_api::TryConvertCount(max_depth, converted_max_depth)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return blitzar_sdk_api::ApplyBarnesHut(
        *simulation, {opening_angle, converted_max_particles, converted_max_cells,
                         converted_leaf_capacity, converted_max_depth});
}

extern "C" blitzar_status blitzar_simulation_set_timestep(
    blitzar_simulation* simulation, double timestep)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetTimestep(timestep);
}

extern "C" blitzar_status blitzar_simulation_set_seed(
    blitzar_simulation* simulation, std::uint64_t seed)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetSeed(seed);
}

extern "C" blitzar_status blitzar_simulation_set_barnes_hut_v2(
    blitzar_simulation* simulation, const blitzar_barnes_hut_config_v2* config)
{
    if (simulation == nullptr || config == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    blitzar_barnes_hut::BarnesHutSettings settings{};

    if (!blitzar_sdk_api::ConvertBarnesHutConfig(*config, settings)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return blitzar_sdk_api::ApplyBarnesHut(*simulation, settings);
}
