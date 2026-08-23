#include "integration/LeapfrogWorkspace.hpp"

#include <utility>

namespace blitzar_integration {

LeapfrogWorkspace::LeapfrogWorkspace(std::size_t count)
    : owned_arena_(std::make_unique<blitzar_particles::ParticleArena>(count)), borrowed_arena_(),
      count_(count)
{
}

LeapfrogWorkspace::LeapfrogWorkspace(blitzar_particles::ParticleArena& arena)
    : owned_arena_(), borrowed_arena_(std::ref(arena)), count_(arena.Count())
{
}

LeapfrogWorkspace::LeapfrogWorkspace(LeapfrogWorkspace&& other) noexcept
    : owned_arena_(std::move(other.owned_arena_)), borrowed_arena_(other.borrowed_arena_),
      count_(other.count_)
{
    other.borrowed_arena_.reset();
    other.count_ = 0;
}

LeapfrogWorkspace& LeapfrogWorkspace::operator=(LeapfrogWorkspace&& other) noexcept
{
    if (this != &other) {
        owned_arena_ = std::move(other.owned_arena_);
        borrowed_arena_ = other.borrowed_arena_;
        count_ = other.count_;
        other.borrowed_arena_.reset();
        other.count_ = 0;
    }
    return *this;
}

bool LeapfrogWorkspace::HasArena() const noexcept
{
    return owned_arena_ != nullptr || borrowed_arena_.has_value();
}

blitzar_particles::ParticleArena& LeapfrogWorkspace::Arena() const noexcept
{
    return owned_arena_ != nullptr ? *owned_arena_ : borrowed_arena_->get();
}

std::size_t LeapfrogWorkspace::Count() const noexcept
{
    return count_;
}

blitzar_status LeapfrogWorkspace::SetCount(std::size_t count) noexcept
{
    if (!HasArena() || count > Arena().Count()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    count_ = count;
    return BLITZAR_STATUS_OK;
}

bool LeapfrogWorkspace::IsValid() const noexcept
{
    return !HasArena() ? count_ == 0 : count_ <= Arena().Count() && Arena().IsValid();
}

blitzar_status LeapfrogWorkspace::Capture(blitzar_core::MutableParticleView state) noexcept
{
    if (state.count != count_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!HasArena()) {
        return BLITZAR_STATUS_OK;
    }
    blitzar_particles::ParticleArena& arena = Arena();
    const auto position_x = arena.WorkspacePositionX();
    const auto position_y = arena.WorkspacePositionY();
    const auto position_z = arena.WorkspacePositionZ();
    const auto velocity_x = arena.WorkspaceVelocityX();
    const auto velocity_y = arena.WorkspaceVelocityY();
    const auto velocity_z = arena.WorkspaceVelocityZ();
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

blitzar_status LeapfrogWorkspace::Restore(blitzar_core::MutableParticleView state) noexcept
{
    if (state.count != count_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!HasArena()) {
        return BLITZAR_STATUS_OK;
    }
    blitzar_particles::ParticleArena& arena = Arena();
    const auto position_x = arena.WorkspacePositionX();
    const auto position_y = arena.WorkspacePositionY();
    const auto position_z = arena.WorkspacePositionZ();
    const auto velocity_x = arena.WorkspaceVelocityX();
    const auto velocity_y = arena.WorkspaceVelocityY();
    const auto velocity_z = arena.WorkspaceVelocityZ();
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

} // namespace blitzar_integration
