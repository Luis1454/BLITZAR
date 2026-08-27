#include "sdk/c/CApiState.hpp"

extern "C" blitzar_status blitzar_simulation_set_particles(blitzar_simulation* simulation,
    std::int64_t particle_count, const double* position_x, const double* position_y,
    const double* position_z, const double* velocity_x, const double* velocity_y,
    const double* velocity_z, const double* mass)
{
    if (simulation == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t converted_count = 0;

    if (!blitzar_sdk_api::TryConvertCount(particle_count, converted_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (converted_count > 0 &&
        (position_x == nullptr || position_y == nullptr || position_z == nullptr ||
            velocity_x == nullptr || velocity_y == nullptr || velocity_z == nullptr ||
            mass == nullptr)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return blitzar_sdk_api::ApplyParticles(
        *simulation, {converted_count, blitzar_sdk_api::MakeSpan(position_x, converted_count),
                         blitzar_sdk_api::MakeSpan(position_y, converted_count),
                         blitzar_sdk_api::MakeSpan(position_z, converted_count),
                         blitzar_sdk_api::MakeSpan(velocity_x, converted_count),
                         blitzar_sdk_api::MakeSpan(velocity_y, converted_count),
                         blitzar_sdk_api::MakeSpan(velocity_z, converted_count),
                         blitzar_sdk_api::MakeSpan(mass, converted_count), converted_count});
}

extern "C" blitzar_status blitzar_simulation_get_state(const blitzar_simulation* simulation,
    std::int64_t capacity, double* position_x, double* position_y, double* position_z,
    double* velocity_x, double* velocity_y, double* velocity_z, double* mass)
{
    if (!blitzar_sdk_api::IsValidSimulation(simulation)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t converted_capacity = 0;

    if (!blitzar_sdk_api::TryConvertCount(capacity, converted_capacity)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (converted_capacity > 0 &&
        (position_x == nullptr || position_y == nullptr || position_z == nullptr ||
            velocity_x == nullptr || velocity_y == nullptr || velocity_z == nullptr ||
            mass == nullptr)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return blitzar_sdk_api::ApplyState(
        *simulation, {converted_capacity, blitzar_sdk_api::MakeSpan(position_x, converted_capacity),
                         blitzar_sdk_api::MakeSpan(position_y, converted_capacity),
                         blitzar_sdk_api::MakeSpan(position_z, converted_capacity),
                         blitzar_sdk_api::MakeSpan(velocity_x, converted_capacity),
                         blitzar_sdk_api::MakeSpan(velocity_y, converted_capacity),
                         blitzar_sdk_api::MakeSpan(velocity_z, converted_capacity),
                         blitzar_sdk_api::MakeSpan(mass, converted_capacity)});
}

extern "C" blitzar_status blitzar_simulation_set_particles_v2(
    blitzar_simulation* simulation, const blitzar_particle_input_v2* input)
{
    if (simulation == nullptr || input == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    blitzar_core::ParticleStateView view{};

    if (!blitzar_sdk_api::ConvertParticleInput(*input, view)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return blitzar_sdk_api::ApplyParticles(*simulation, view);
}

extern "C" blitzar_status blitzar_simulation_get_state_v2(
    const blitzar_simulation* simulation, const blitzar_particle_output_v2* output)
{
    if (!blitzar_sdk_api::IsValidSimulation(simulation) || output == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_sdk_api::SimulationCallGuard guard(*simulation);

    if (!guard.Acquired()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    blitzar_core::ParticleOutputView view{};

    if (!blitzar_sdk_api::ConvertParticleOutput(*output, view)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return blitzar_sdk_api::ApplyState(*simulation, view);
}
