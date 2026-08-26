#include "particles/buffers/ParticleBuffer.hpp"

#include <cmath>

namespace blitzar_particles {

blitzar_core::ParticleStateView ParticleBuffer::State() const noexcept
{
    if (!HasArena()) {
        return {};
    }

    ParticleArena& arena = Arena();

    return {count_, arena.PositionX().first(count_), arena.PositionY().first(count_),
        arena.PositionZ().first(count_), arena.VelocityX().first(count_),
        arena.VelocityY().first(count_), arena.VelocityZ().first(count_),
        arena.Mass().first(count_)};
}

blitzar_core::MutableParticleView ParticleBuffer::MutableView() noexcept
{
    if (!HasArena()) {
        return {};
    }

    ParticleArena& arena = Arena();

    return {count_, arena.PositionX().first(count_), arena.PositionY().first(count_),
        arena.PositionZ().first(count_), arena.VelocityX().first(count_),
        arena.VelocityY().first(count_), arena.VelocityZ().first(count_)};
}

blitzar_status ParticleBuffer::SetPosition(
    std::size_t index, blitzar_core::Vector3 position) noexcept
{
    if (!HasArena() || index >= count_ || !std::isfinite(position.x) ||
        !std::isfinite(position.y) || !std::isfinite(position.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    ParticleArena& arena = Arena();

    const auto x = arena.PositionX();
    const auto y = arena.PositionY();
    const auto z = arena.PositionZ();

    x[index] = position.x;
    y[index] = position.y;
    z[index] = position.z;

    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleBuffer::SetVelocity(
    std::size_t index, blitzar_core::Vector3 velocity) noexcept
{
    if (!HasArena() || index >= count_ || !std::isfinite(velocity.x) ||
        !std::isfinite(velocity.y) || !std::isfinite(velocity.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    ParticleArena& arena = Arena();

    const auto x = arena.VelocityX();
    const auto y = arena.VelocityY();
    const auto z = arena.VelocityZ();

    x[index] = velocity.x;
    y[index] = velocity.y;
    z[index] = velocity.z;

    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleBuffer::SetMass(std::size_t index, blitzar_core::Scalar mass) noexcept
{
    if (!HasArena() || index >= count_ || !std::isfinite(mass) || mass < 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    Arena().Mass()[index] = mass;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_particles
