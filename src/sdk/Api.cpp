#include "sdk/Simulation.hpp"

#include <atomic>
#include <blitzar/blitzar.h>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>

#ifndef BLITZAR_BUILD_PRODUCT_VERSION
#error "BLITZAR_BUILD_PRODUCT_VERSION must be supplied by CMake"
#endif

#ifndef BLITZAR_BUILD_PLAN_VERSION
#error "BLITZAR_BUILD_PLAN_VERSION must be supplied by CMake"
#endif

struct blitzar_context {
    blitzar_status status;
};

struct blitzar_simulation final {
    explicit blitzar_simulation(std::size_t particle_count) : implementation(particle_count) {}

    blitzar_sdk::Simulation implementation;
    mutable std::atomic_flag call_active = ATOMIC_FLAG_INIT;
};

namespace {

class SimulationCallGuard final {
public:
    explicit SimulationCallGuard(const blitzar_simulation& simulation) noexcept
        : simulation_(simulation),
          acquired_(!simulation_.call_active.test_and_set(std::memory_order_acquire))
    {
    }

    ~SimulationCallGuard() noexcept
    {
        if (acquired_) {
            simulation_.call_active.clear(std::memory_order_release);
        }
    }

    SimulationCallGuard(const SimulationCallGuard&) = delete;
    SimulationCallGuard& operator=(const SimulationCallGuard&) = delete;

    [[nodiscard]] bool Acquired() const noexcept
    {
        return acquired_;
    }

private:
    const blitzar_simulation& simulation_;
    bool acquired_;
};

[[nodiscard]] bool TryConvertCount(int64_t value, std::size_t& converted) noexcept
{
    if (value < 0 || static_cast<std::uint64_t>(value) >
                         static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return false;
    }

    converted = static_cast<std::size_t>(value);

    return true;
}

[[nodiscard]] bool IsValidSimulation(const blitzar_simulation* simulation) noexcept
{
    return simulation != nullptr;
}

template <typename Scalar>
[[nodiscard]] std::span<Scalar> MakeSpan(Scalar* data, std::size_t count) noexcept
{
    return count == 0 ? std::span<Scalar>{} : std::span<Scalar>(data, count);
}

} // namespace

extern "C" blitzar_status blitzar_context_create(blitzar_context** context)
{
    if (context == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    *context = nullptr;

    try {
        *context = new blitzar_context{BLITZAR_STATUS_OK};
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

extern "C" const char* blitzar_version(void)
{
    return BLITZAR_BUILD_PRODUCT_VERSION;
}

extern "C" const char* blitzar_plan_version(void)
{
    return BLITZAR_BUILD_PLAN_VERSION;
}

extern "C" void blitzar_context_destroy(blitzar_context* context)
{
    delete context;
}

extern "C" blitzar_status blitzar_context_status(const blitzar_context* context)
{
    if (context == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return context->status;
}

extern "C" const char* blitzar_status_message(blitzar_status status)
{
    switch (status) {
    case BLITZAR_STATUS_OK:

        return "ok";

    case BLITZAR_STATUS_INVALID_ARGUMENT:

        return "invalid argument";

    case BLITZAR_STATUS_ALLOCATION_FAILURE:

        return "allocation failure";

    case BLITZAR_STATUS_INTERNAL_ERROR:

        return "internal error";

    case BLITZAR_STATUS_SINGULARITY:

        return "gravitational singularity";

    case BLITZAR_STATUS_UNSUPPORTED:

        return "unsupported";

    default:

        return "unknown status";
    }
}

extern "C" blitzar_status blitzar_simulation_create(
    blitzar_context* context, int64_t particle_count, blitzar_simulation** simulation)
{
    if (context == nullptr || context->status != BLITZAR_STATUS_OK || simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    *simulation = nullptr;
    std::size_t converted_count = 0;
    if (!TryConvertCount(particle_count, converted_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        *simulation = new blitzar_simulation(converted_count);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    return BLITZAR_STATUS_OK;
}

extern "C" void blitzar_simulation_destroy(blitzar_simulation* simulation)
{
    delete simulation;
}

extern "C" blitzar_status blitzar_simulation_status(const blitzar_simulation* simulation)
{
    if (!IsValidSimulation(simulation)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.LastStatus();
}

extern "C" blitzar_status blitzar_simulation_backend(
    const blitzar_simulation* simulation, blitzar_backend_kind* backend)
{
    if (!IsValidSimulation(simulation) || backend == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    *backend = simulation->implementation.LastBackend();
    return BLITZAR_STATUS_OK;
}

extern "C" blitzar_status blitzar_simulation_particle_count(
    const blitzar_simulation* simulation, int64_t* particle_count)
{
    if (!IsValidSimulation(simulation) || particle_count == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    *particle_count = static_cast<int64_t>(simulation->implementation.ParticleCount());
    return BLITZAR_STATUS_OK;
}

extern "C" blitzar_status blitzar_simulation_set_solver(
    blitzar_simulation* simulation, blitzar_solver_kind solver)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
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
    const SimulationCallGuard guard(*simulation);
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
    const SimulationCallGuard guard(*simulation);
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
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    return simulation->implementation.SetUnits({length_scale, mass_scale, time_scale});
}

extern "C" blitzar_status blitzar_simulation_set_barnes_hut(blitzar_simulation* simulation,
    double opening_angle, int64_t max_particles, int64_t max_cells, int64_t leaf_capacity,
    int64_t max_depth)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    std::size_t converted_max_particles = 0;
    std::size_t converted_max_cells = 0;
    std::size_t converted_leaf_capacity = 0;
    std::size_t converted_max_depth = 0;
    if (!TryConvertCount(max_particles, converted_max_particles) ||
        !TryConvertCount(max_cells, converted_max_cells) ||
        !TryConvertCount(leaf_capacity, converted_leaf_capacity) ||
        !TryConvertCount(max_depth, converted_max_depth)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return simulation->implementation.SetBarnesHut(opening_angle, converted_max_particles,
        converted_max_cells, converted_leaf_capacity, converted_max_depth);
}

extern "C" blitzar_status blitzar_simulation_set_timestep(
    blitzar_simulation* simulation, double timestep)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    return simulation->implementation.SetTimestep(timestep);
}

extern "C" blitzar_status blitzar_simulation_set_seed(blitzar_simulation* simulation, uint64_t seed)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.SetSeed(seed);
}

extern "C" blitzar_status blitzar_simulation_set_particles(blitzar_simulation* simulation,
    int64_t particle_count, const double* position_x, const double* position_y,
    const double* position_z, const double* velocity_x, const double* velocity_y,
    const double* velocity_z, const double* mass)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    std::size_t converted_count = 0;
    if (!TryConvertCount(particle_count, converted_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (converted_count > 0 &&
        (position_x == nullptr || position_y == nullptr || position_z == nullptr ||
            velocity_x == nullptr || velocity_y == nullptr || velocity_z == nullptr ||
            mass == nullptr)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return simulation->implementation.SetParticles(MakeSpan(position_x, converted_count),
        MakeSpan(position_y, converted_count), MakeSpan(position_z, converted_count),
        MakeSpan(velocity_x, converted_count), MakeSpan(velocity_y, converted_count),
        MakeSpan(velocity_z, converted_count), MakeSpan(mass, converted_count));
}

extern "C" blitzar_status blitzar_simulation_get_state(const blitzar_simulation* simulation,
    int64_t capacity, double* position_x, double* position_y, double* position_z,
    double* velocity_x, double* velocity_y, double* velocity_z, double* mass)
{
    if (!IsValidSimulation(simulation)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const SimulationCallGuard guard(*simulation);
    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    std::size_t converted_capacity = 0;
    if (!TryConvertCount(capacity, converted_capacity)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (converted_capacity > 0 &&
        (position_x == nullptr || position_y == nullptr || position_z == nullptr ||
            velocity_x == nullptr || velocity_y == nullptr || velocity_z == nullptr ||
            mass == nullptr)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return simulation->implementation.GetState(MakeSpan(position_x, converted_capacity),
        MakeSpan(position_y, converted_capacity), MakeSpan(position_z, converted_capacity),
        MakeSpan(velocity_x, converted_capacity), MakeSpan(velocity_y, converted_capacity),
        MakeSpan(velocity_z, converted_capacity), MakeSpan(mass, converted_capacity));
}

extern "C" blitzar_status blitzar_simulation_step(blitzar_simulation* simulation)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return simulation->implementation.Step();
}
