#include "sdk/ApiState.hpp"

#include <limits>

namespace blitzar_sdk_api {

bool TryConvertCount(std::int64_t value, std::size_t& converted) noexcept
{
    if (value < 0 || static_cast<std::uint64_t>(value) >
                         static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return false;
    }

    converted = static_cast<std::size_t>(value);

    return true;
}

bool IsValidSimulation(const blitzar_simulation* simulation) noexcept
{
    return simulation != nullptr;
}

bool HasV2Header(
    std::uint32_t struct_size, std::uint32_t abi_version, std::size_t minimum_size) noexcept
{
    return abi_version == BLITZAR_ABI_VERSION_V2 &&
           static_cast<std::size_t>(struct_size) >= minimum_size;
}

bool ConvertBarnesHutConfig(const blitzar_barnes_hut_config_v2& source,
    blitzar_barnes_hut::BarnesHutSettings& target) noexcept
{
    std::size_t max_particles = 0;
    std::size_t max_cells = 0;
    std::size_t leaf_capacity = 0;
    std::size_t max_depth = 0;

    if (!HasV2Header(source.struct_size, source.abi_version, sizeof(source)) ||
        !TryConvertCount(source.max_particles, max_particles) ||
        !TryConvertCount(source.max_cells, max_cells) ||
        !TryConvertCount(source.leaf_capacity, leaf_capacity) ||
        !TryConvertCount(source.max_depth, max_depth)) {
        return false;
    }

    target = {source.opening_angle, max_particles, max_cells, leaf_capacity, max_depth};

    return true;
}

bool ConvertParticleInput(
    const blitzar_particle_input_v2& source, blitzar_core::ParticleStateView& target) noexcept
{
    std::size_t count = 0;

    if (!HasV2Header(source.struct_size, source.abi_version, sizeof(source)) ||
        !TryConvertCount(source.particle_count, count)) {
        return false;
    }
    if (count > 0 && (source.position_x == nullptr || source.position_y == nullptr ||
                         source.position_z == nullptr || source.velocity_x == nullptr ||
                         source.velocity_y == nullptr || source.velocity_z == nullptr ||
                         source.mass == nullptr)) {
        return false;
    }

    target = {count, MakeSpan(source.position_x, count), MakeSpan(source.position_y, count),
        MakeSpan(source.position_z, count), MakeSpan(source.velocity_x, count),
        MakeSpan(source.velocity_y, count), MakeSpan(source.velocity_z, count),
        MakeSpan(source.mass, count), count};

    return blitzar_core::IsValid(target);
}

bool ConvertParticleOutput(
    const blitzar_particle_output_v2& source, blitzar_core::ParticleOutputView& target) noexcept
{
    std::size_t capacity = 0;

    if (!HasV2Header(source.struct_size, source.abi_version, sizeof(source)) ||
        !TryConvertCount(source.capacity, capacity)) {
        return false;
    }
    if (capacity > 0 && (source.position_x == nullptr || source.position_y == nullptr ||
                            source.position_z == nullptr || source.velocity_x == nullptr ||
                            source.velocity_y == nullptr || source.velocity_z == nullptr ||
                            source.mass == nullptr)) {
        return false;
    }

    target = {capacity, MakeSpan(source.position_x, capacity),
        MakeSpan(source.position_y, capacity), MakeSpan(source.position_z, capacity),
        MakeSpan(source.velocity_x, capacity), MakeSpan(source.velocity_y, capacity),
        MakeSpan(source.velocity_z, capacity), MakeSpan(source.mass, capacity)};

    return blitzar_core::IsValid(target);
}

blitzar_status ApplyBarnesHut(
    blitzar_simulation& simulation, blitzar_barnes_hut::BarnesHutSettings settings) noexcept
{
    return simulation.implementation.SetBarnesHut(settings);
}

blitzar_status ApplyParticles(
    blitzar_simulation& simulation, blitzar_core::ParticleStateView input) noexcept
{
    return simulation.implementation.SetParticles(input);
}

blitzar_status ApplyState(
    const blitzar_simulation& simulation, blitzar_core::ParticleOutputView output) noexcept
{
    return simulation.implementation.GetState(output);
}

} // namespace blitzar_sdk_api
