#include "particles/ParticleBuffer.hpp"

#include <cmath>

namespace blitzar_particles {

ParticleBuffer::ParticleBuffer(std::size_t count)
    : count_(count),
      position_x_(count),
      position_y_(count),
      position_z_(count),
      velocity_x_(count),
      velocity_y_(count),
      velocity_z_(count),
      mass_(count)
{
    mass_.Fill(1.0);
}

std::size_t ParticleBuffer::Count() const noexcept
{
    return count_;
}

bool ParticleBuffer::IsValid() const noexcept
{
    return count_ == position_x_.Size() && count_ == position_y_.Size() &&
           count_ == position_z_.Size() && count_ == velocity_x_.Size() &&
           count_ == velocity_y_.Size() && count_ == velocity_z_.Size() &&
           count_ == mass_.Size();
}

blitzar_core::ParticleStateView ParticleBuffer::State() const noexcept
{
    return {count_, position_x_.Data(), position_y_.Data(), position_z_.Data(),
            velocity_x_.Data(), velocity_y_.Data(), velocity_z_.Data(),
            mass_.Data()};
}

blitzar_core::MutableParticleView ParticleBuffer::MutableView() noexcept
{
    return {count_, position_x_.Data(), position_y_.Data(), position_z_.Data(),
            velocity_x_.Data(), velocity_y_.Data(), velocity_z_.Data()};
}

blitzar_status ParticleBuffer::SetPosition(
    std::size_t index, blitzar_core::Vector3 position) noexcept
{
    if (index >= count_ || !std::isfinite(position.x) ||
        !std::isfinite(position.y) || !std::isfinite(position.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    position_x_.Data()[index] = position.x;
    position_y_.Data()[index] = position.y;
    position_z_.Data()[index] = position.z;
    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleBuffer::SetVelocity(
    std::size_t index, blitzar_core::Vector3 velocity) noexcept
{
    if (index >= count_ || !std::isfinite(velocity.x) ||
        !std::isfinite(velocity.y) || !std::isfinite(velocity.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    velocity_x_.Data()[index] = velocity.x;
    velocity_y_.Data()[index] = velocity.y;
    velocity_z_.Data()[index] = velocity.z;
    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleBuffer::SetMass(
    std::size_t index, blitzar_core::Scalar mass) noexcept
{
    if (index >= count_ || !std::isfinite(mass) || mass < 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    mass_.Data()[index] = mass;
    return BLITZAR_STATUS_OK;
}

AccelerationBuffer::AccelerationBuffer(std::size_t count)
    : count_(count), x_(count), y_(count), z_(count)
{
}

std::size_t AccelerationBuffer::Count() const noexcept
{
    return count_;
}

blitzar_core::ForceView AccelerationBuffer::View() noexcept
{
    return {count_, x_.Data(), y_.Data(), z_.Data()};
}

}  // namespace blitzar_particles
