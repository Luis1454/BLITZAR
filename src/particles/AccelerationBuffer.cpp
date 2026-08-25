#include "particles/ParticleBuffer.hpp"

#include <utility>

namespace blitzar_particles {

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
