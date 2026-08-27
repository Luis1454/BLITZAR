#include "simulation/staging/SimParticleStage.hpp"

#include <cmath>
#include <new>
#include <stdexcept>

namespace blitzar_sim {

namespace {

[[nodiscard]] bool IsFiniteParticle(
    blitzar_core::ParticleStateView source, std::size_t index) noexcept
{
    return std::isfinite(source.x[index]) && std::isfinite(source.y[index]) &&
           std::isfinite(source.z[index]) && std::isfinite(source.velocity_x[index]) &&
           std::isfinite(source.velocity_y[index]) && std::isfinite(source.velocity_z[index]) &&
           std::isfinite(source.mass[index]) && source.mass[index] >= 0.0;
}

[[nodiscard]] blitzar_status ResizeStage(std::size_t count, SimParticleStage& stage) noexcept
{
    try {
        stage.position_x.resize(count);
        stage.position_y.resize(count);
        stage.position_z.resize(count);
        stage.velocity_x.resize(count);
        stage.velocity_y.resize(count);
        stage.velocity_z.resize(count);
        stage.mass.resize(count);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

void CopyParticle(
    blitzar_core::ParticleStateView source, std::size_t index, SimParticleStage& stage) noexcept
{
    stage.position_x[index] = source.x[index];
    stage.position_y[index] = source.y[index];
    stage.position_z[index] = source.z[index];
    stage.velocity_x[index] = source.velocity_x[index];
    stage.velocity_y[index] = source.velocity_y[index];
    stage.velocity_z[index] = source.velocity_z[index];
    stage.mass[index] = source.mass[index];
}

} // namespace

blitzar_core::ParticleStateView SimParticleStage::State() const noexcept
{
    return {position_x.size(), std::span<const blitzar_core::Scalar>(position_x),
        std::span<const blitzar_core::Scalar>(position_y),
        std::span<const blitzar_core::Scalar>(position_z),
        std::span<const blitzar_core::Scalar>(velocity_x),
        std::span<const blitzar_core::Scalar>(velocity_y),
        std::span<const blitzar_core::Scalar>(velocity_z),
        std::span<const blitzar_core::Scalar>(mass), position_x.size()};
}

blitzar_status StageParticles(
    blitzar_core::ParticleStateView source, SimParticleStage& stage) noexcept
{
    if (!blitzar_core::IsValid(source)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status resize_status = ResizeStage(source.count, stage);

    if (resize_status != BLITZAR_STATUS_OK) {
        return resize_status;
    }

    for (std::size_t index = 0; index < source.count; ++index) {
        if (!IsFiniteParticle(source, index)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        CopyParticle(source, index, stage);
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
