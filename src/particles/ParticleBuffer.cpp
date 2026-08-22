#include "particles/ParticleBuffer.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace blitzar_particles {

ParticleBuffer::ParticleBuffer(std::size_t count)
    : ParticleBuffer(std::make_shared<ParticleArena>(count))
{
}

ParticleBuffer::ParticleBuffer(std::shared_ptr<ParticleArena> arena)
    : count_(arena == nullptr ? 0 : arena->Count()), arena_(std::move(arena))
{
    if (arena_ == nullptr) {
        throw std::invalid_argument("particle arena is required");
    }
    const auto mass = arena_->Mass();
    std::fill(mass.begin(), mass.end(), 1.0);
}

ParticleBuffer::ParticleBuffer(ParticleBuffer&& other) noexcept
    : count_(other.count_), arena_(std::move(other.arena_))
{
    other.count_ = 0;
}

ParticleBuffer& ParticleBuffer::operator=(ParticleBuffer&& other) noexcept
{
    if (this != &other) {
        count_ = other.count_;
        arena_ = std::move(other.arena_);
        other.count_ = 0;
    }
    return *this;
}

std::size_t ParticleBuffer::Count() const noexcept
{
    return count_;
}

blitzar_status ParticleBuffer::SetCount(std::size_t count) noexcept
{
    if (arena_ == nullptr || count > arena_->Count()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    count_ = count;
    return BLITZAR_STATUS_OK;
}

bool ParticleBuffer::IsValid() const noexcept
{
    return (arena_ == nullptr && count_ == 0) ||
           (arena_ != nullptr && count_ <= arena_->Count() && arena_->IsValid());
}

blitzar_core::ParticleStateView ParticleBuffer::State() const noexcept
{
    if (arena_ == nullptr) {
        return {};
    }
    return {
        count_,
        arena_->PositionX().first(count_),
        arena_->PositionY().first(count_),
        arena_->PositionZ().first(count_),
        arena_->VelocityX().first(count_),
        arena_->VelocityY().first(count_),
        arena_->VelocityZ().first(count_),
        arena_->Mass().first(count_)};
}

blitzar_core::MutableParticleView ParticleBuffer::MutableView() noexcept
{
    if (arena_ == nullptr) {
        return {};
    }
    return {
        count_,
        arena_->PositionX().first(count_),
        arena_->PositionY().first(count_),
        arena_->PositionZ().first(count_),
        arena_->VelocityX().first(count_),
        arena_->VelocityY().first(count_),
        arena_->VelocityZ().first(count_)};
}

blitzar_status ParticleBuffer::SetPosition(
    std::size_t index, blitzar_core::Vector3 position) noexcept
{
    if (index >= count_ || !std::isfinite(position.x) ||
        !std::isfinite(position.y) || !std::isfinite(position.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const auto x = arena_->PositionX();
    const auto y = arena_->PositionY();
    const auto z = arena_->PositionZ();
    x[index] = position.x;
    y[index] = position.y;
    z[index] = position.z;
    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleBuffer::SetVelocity(
    std::size_t index, blitzar_core::Vector3 velocity) noexcept
{
    if (index >= count_ || !std::isfinite(velocity.x) ||
        !std::isfinite(velocity.y) || !std::isfinite(velocity.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    const auto x = arena_->VelocityX();
    const auto y = arena_->VelocityY();
    const auto z = arena_->VelocityZ();
    x[index] = velocity.x;
    y[index] = velocity.y;
    z[index] = velocity.z;
    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleBuffer::SetMass(
    std::size_t index, blitzar_core::Scalar mass) noexcept
{
    if (index >= count_ || !std::isfinite(mass) || mass < 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    arena_->Mass()[index] = mass;
    return BLITZAR_STATUS_OK;
}

AccelerationBuffer::AccelerationBuffer(std::size_t count)
    : AccelerationBuffer(std::make_shared<ParticleArena>(count))
{
}

AccelerationBuffer::AccelerationBuffer(std::shared_ptr<ParticleArena> arena)
    : count_(arena == nullptr ? 0 : arena->Count()), arena_(std::move(arena))
{
    if (arena_ == nullptr) {
        throw std::invalid_argument("particle arena is required");
    }
}

AccelerationBuffer::AccelerationBuffer(AccelerationBuffer&& other) noexcept
    : count_(other.count_), arena_(std::move(other.arena_))
{
    other.count_ = 0;
}

AccelerationBuffer& AccelerationBuffer::operator=(
    AccelerationBuffer&& other) noexcept
{
    if (this != &other) {
        count_ = other.count_;
        arena_ = std::move(other.arena_);
        other.count_ = 0;
    }
    return *this;
}

std::size_t AccelerationBuffer::Count() const noexcept
{
    return count_;
}

blitzar_status AccelerationBuffer::SetCount(std::size_t count) noexcept
{
    if (arena_ == nullptr || count > arena_->Count()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    count_ = count;
    return BLITZAR_STATUS_OK;
}

bool AccelerationBuffer::IsValid() const noexcept
{
    return (arena_ == nullptr && count_ == 0) ||
           (arena_ != nullptr && count_ <= arena_->Count() && arena_->IsValid());
}

blitzar_core::ForceView AccelerationBuffer::View() noexcept
{
    if (arena_ == nullptr) {
        return {};
    }
    return {
        count_,
        arena_->AccelerationX().first(count_),
        arena_->AccelerationY().first(count_),
        arena_->AccelerationZ().first(count_)};
}

}  // namespace blitzar_particles
