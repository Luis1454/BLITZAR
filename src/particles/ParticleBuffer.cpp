#include "particles/ParticleBuffer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace blitzar_particles {

ParticleBuffer::ParticleBuffer(std::size_t count)
    : owned_arena_(std::make_unique<ParticleArena>(count)), borrowed_arena_(), count_(count)
{
    const auto mass = owned_arena_->Mass();
    std::fill(mass.begin(), mass.end(), 1.0);
}

ParticleBuffer::ParticleBuffer(ParticleArena& arena)
    : owned_arena_(), borrowed_arena_(std::ref(arena)), count_(arena.Count())
{
    const auto mass = arena.Mass();
    std::fill(mass.begin(), mass.end(), 1.0);
}

ParticleBuffer::ParticleBuffer(ParticleBuffer&& other) noexcept
    : owned_arena_(std::move(other.owned_arena_)), borrowed_arena_(other.borrowed_arena_),
      count_(other.count_)
{
    other.borrowed_arena_.reset();
    other.count_ = 0;
}

ParticleBuffer& ParticleBuffer::operator=(ParticleBuffer&& other) noexcept
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

bool ParticleBuffer::HasArena() const noexcept
{
    return owned_arena_ != nullptr || borrowed_arena_.has_value();
}

ParticleArena& ParticleBuffer::Arena() const noexcept
{
    return owned_arena_ != nullptr ? *owned_arena_ : borrowed_arena_->get();
}

std::size_t ParticleBuffer::Count() const noexcept
{
    return count_;
}

blitzar_status ParticleBuffer::SetCount(std::size_t count) noexcept
{
    if (!HasArena() || count > Arena().Count()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    count_ = count;
    return BLITZAR_STATUS_OK;
}

bool ParticleBuffer::IsValid() const noexcept
{
    return !HasArena() ? count_ == 0 : count_ <= Arena().Count() && Arena().IsValid();
}

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

AccelerationBuffer::AccelerationBuffer(std::size_t count)
    : owned_arena_(std::make_unique<ParticleArena>(count)), borrowed_arena_(), count_(count)
{
}

AccelerationBuffer::AccelerationBuffer(ParticleArena& arena)
    : owned_arena_(), borrowed_arena_(std::ref(arena)), count_(arena.Count())
{
}

AccelerationBuffer::AccelerationBuffer(AccelerationBuffer&& other) noexcept
    : owned_arena_(std::move(other.owned_arena_)), borrowed_arena_(other.borrowed_arena_),
      count_(other.count_)
{
    other.borrowed_arena_.reset();
    other.count_ = 0;
}

AccelerationBuffer& AccelerationBuffer::operator=(AccelerationBuffer&& other) noexcept
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

bool AccelerationBuffer::HasArena() const noexcept
{
    return owned_arena_ != nullptr || borrowed_arena_.has_value();
}

ParticleArena& AccelerationBuffer::Arena() const noexcept
{
    return owned_arena_ != nullptr ? *owned_arena_ : borrowed_arena_->get();
}

std::size_t AccelerationBuffer::Count() const noexcept
{
    return count_;
}

blitzar_status AccelerationBuffer::SetCount(std::size_t count) noexcept
{
    if (!HasArena() || count > Arena().Count()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    count_ = count;
    return BLITZAR_STATUS_OK;
}

bool AccelerationBuffer::IsValid() const noexcept
{
    return !HasArena() ? count_ == 0 : count_ <= Arena().Count() && Arena().IsValid();
}

blitzar_core::ForceView AccelerationBuffer::View() noexcept
{
    if (!HasArena()) {
        return {};
    }
    ParticleArena& arena = Arena();
    return {count_, arena.AccelerationX().first(count_), arena.AccelerationY().first(count_),
        arena.AccelerationZ().first(count_)};
}

} // namespace blitzar_particles
