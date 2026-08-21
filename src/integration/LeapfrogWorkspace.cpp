#include "integration/LeapfrogWorkspace.hpp"

#include <stdexcept>
#include <utility>

namespace blitzar_integration {

LeapfrogWorkspace::LeapfrogWorkspace(std::size_t count)
    : LeapfrogWorkspace(std::make_shared<blitzar_particles::ParticleArena>(count))
{
}

LeapfrogWorkspace::LeapfrogWorkspace(
    std::shared_ptr<blitzar_particles::ParticleArena> arena)
    : count_(arena == nullptr ? 0 : arena->Count()), arena_(std::move(arena))
{
    if (arena_ == nullptr) {
        throw std::invalid_argument("particle arena is required");
    }
}

LeapfrogWorkspace::LeapfrogWorkspace(LeapfrogWorkspace&& other) noexcept
    : count_(other.count_), arena_(std::move(other.arena_))
{
    other.count_ = 0;
}

LeapfrogWorkspace& LeapfrogWorkspace::operator=(
    LeapfrogWorkspace&& other) noexcept
{
    if (this != &other) {
        count_ = other.count_;
        arena_ = std::move(other.arena_);
        other.count_ = 0;
    }
    return *this;
}

std::size_t LeapfrogWorkspace::Count() const noexcept
{
    return count_;
}

bool LeapfrogWorkspace::IsValid() const noexcept
{
    return (arena_ == nullptr && count_ == 0) ||
           (arena_ != nullptr && count_ == arena_->Count() && arena_->IsValid());
}

blitzar_status LeapfrogWorkspace::Capture(
    blitzar_core::MutableParticleView state) noexcept
{
    if (state.count != count_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (arena_ == nullptr) {
        return BLITZAR_STATUS_OK;
    }
    const auto position_x = arena_->WorkspacePositionX();
    const auto position_y = arena_->WorkspacePositionY();
    const auto position_z = arena_->WorkspacePositionZ();
    const auto velocity_x = arena_->WorkspaceVelocityX();
    const auto velocity_y = arena_->WorkspaceVelocityY();
    const auto velocity_z = arena_->WorkspaceVelocityZ();
    for (std::size_t index = 0; index < count_; ++index) {
        position_x[index] = state.x[index];
        position_y[index] = state.y[index];
        position_z[index] = state.z[index];
        velocity_x[index] = state.velocity_x[index];
        velocity_y[index] = state.velocity_y[index];
        velocity_z[index] = state.velocity_z[index];
    }
    return BLITZAR_STATUS_OK;
}

blitzar_status LeapfrogWorkspace::Restore(
    blitzar_core::MutableParticleView state) noexcept
{
    if (state.count != count_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (arena_ == nullptr) {
        return BLITZAR_STATUS_OK;
    }
    const auto position_x = arena_->WorkspacePositionX();
    const auto position_y = arena_->WorkspacePositionY();
    const auto position_z = arena_->WorkspacePositionZ();
    const auto velocity_x = arena_->WorkspaceVelocityX();
    const auto velocity_y = arena_->WorkspaceVelocityY();
    const auto velocity_z = arena_->WorkspaceVelocityZ();
    for (std::size_t index = 0; index < count_; ++index) {
        state.x[index] = position_x[index];
        state.y[index] = position_y[index];
        state.z[index] = position_z[index];
        state.velocity_x[index] = velocity_x[index];
        state.velocity_y[index] = velocity_y[index];
        state.velocity_z[index] = velocity_z[index];
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace blitzar_integration
