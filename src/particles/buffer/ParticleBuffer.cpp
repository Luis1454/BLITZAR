#include "particles/buffer/ParticleBuffer.hpp"

#include <algorithm>
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

} // namespace blitzar_particles
