#include "sdk/ParticleInputStage.hpp"

#include <cmath>
#include <new>
#include <stdexcept>

namespace blitzar_sdk {

namespace {

[[nodiscard]] bool IsFiniteParticle(
    blitzar_core::ParticleStateView input, std::size_t index) noexcept
{
    return std::isfinite(input.x[index]) && std::isfinite(input.y[index]) &&
           std::isfinite(input.z[index]) && std::isfinite(input.velocity_x[index]) &&
           std::isfinite(input.velocity_y[index]) && std::isfinite(input.velocity_z[index]) &&
           std::isfinite(input.mass[index]) && input.mass[index] >= 0.0;
}

[[nodiscard]] blitzar_status ResizeStage(std::size_t count, ParticleInputStage& stage) noexcept
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
    blitzar_core::ParticleStateView input, std::size_t index, ParticleInputStage& stage) noexcept
{
    stage.position_x[index] = input.x[index];
    stage.position_y[index] = input.y[index];
    stage.position_z[index] = input.z[index];
    stage.velocity_x[index] = input.velocity_x[index];
    stage.velocity_y[index] = input.velocity_y[index];
    stage.velocity_z[index] = input.velocity_z[index];
    stage.mass[index] = input.mass[index];
}

} // namespace

blitzar_core::ParticleStateView ParticleInputStage::State() const noexcept
{
    return {position_x.size(), std::span<const blitzar_core::Scalar>(position_x),
        std::span<const blitzar_core::Scalar>(position_y),
        std::span<const blitzar_core::Scalar>(position_z),
        std::span<const blitzar_core::Scalar>(velocity_x),
        std::span<const blitzar_core::Scalar>(velocity_y),
        std::span<const blitzar_core::Scalar>(velocity_z),
        std::span<const blitzar_core::Scalar>(mass), position_x.size()};
}

blitzar_status StageParticleInput(
    blitzar_core::ParticleStateView input, ParticleInputStage& stage) noexcept
{
    if (!blitzar_core::IsValid(input)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status resize_status = ResizeStage(input.count, stage);

    if (resize_status != BLITZAR_STATUS_OK) {
        return resize_status;
    }

    for (std::size_t index = 0; index < input.count; ++index) {
        if (!IsFiniteParticle(input, index)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        CopyParticle(input, index, stage);
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sdk
