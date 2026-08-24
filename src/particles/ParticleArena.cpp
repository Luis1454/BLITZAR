#include "particles/ParticleArena.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_particles {

static_assert(64 % sizeof(blitzar_core::Scalar) == 0,
    "Particle arena alignment must be a multiple of the scalar size");

std::size_t ParticleArena::AlignedCount(std::size_t count)
{
    const std::size_t remainder = count % ScalarsPerAlignment;
    const std::size_t padding = remainder == 0 ? 0 : ScalarsPerAlignment - remainder;

    if (count > std::numeric_limits<std::size_t>::max() - padding) {
        throw std::length_error("particle arena is too large");
    }

    return count + padding;
}

ParticleArena::ParticleArena(std::size_t count) : count_(count), stride_(AlignedCount(count))
{
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();

    if (stride_ > maximum / FieldCount ||
        stride_ * FieldCount > maximum - (ScalarsPerAlignment - 1)) {
        throw std::length_error("particle arena is too large");
    }

    if (count_ == 0) {
        return;
    }

    storage_.resize(stride_ * FieldCount + (ScalarsPerAlignment - 1));

    const std::span<blitzar_core::Scalar> storage(storage_);
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(storage_.data());
    const std::size_t byte_offset = (Alignment - (address % Alignment)) % Alignment;
    const std::size_t scalar_offset = byte_offset / sizeof(blitzar_core::Scalar);
    const std::size_t payload = stride_ * FieldCount;

    if (scalar_offset > storage.size() || payload > storage.size() - scalar_offset) {
        throw std::length_error("particle arena alignment overflow");
    }

    for (std::size_t index = 0; index < FieldCount; ++index) {
        fields_[index] = storage.subspan(scalar_offset + index * stride_, count_);
    }
}

std::size_t ParticleArena::Count() const noexcept
{
    return count_;
}

blitzar_status ParticleArena::Reserve(std::size_t capacity) noexcept
{
    if (capacity <= count_) {
        return BLITZAR_STATUS_OK;
    }

    const std::size_t new_stride = [&]() {
        try {
            return AlignedCount(capacity);
        }
        catch (const std::length_error&) {
            return std::size_t{0};
        }
    }();

    if (new_stride == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t maximum = std::numeric_limits<std::size_t>::max();

    if (new_stride > maximum / FieldCount ||
        new_stride * FieldCount > maximum - (ScalarsPerAlignment - 1)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::vector<blitzar_core::Scalar> candidate;
    std::array<std::span<blitzar_core::Scalar>, FieldCount> candidate_fields{};

    try {
        candidate.resize(new_stride * FieldCount + (ScalarsPerAlignment - 1));

        const std::span<blitzar_core::Scalar> candidate_storage(candidate);
        const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(candidate.data());
        const std::size_t byte_offset = (Alignment - (address % Alignment)) % Alignment;
        const std::size_t scalar_offset = byte_offset / sizeof(blitzar_core::Scalar);
        const std::size_t payload = new_stride * FieldCount;

        if (scalar_offset > candidate_storage.size() ||
            payload > candidate_storage.size() - scalar_offset) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        for (std::size_t index = 0; index < FieldCount; ++index) {
            candidate_fields[index] =
                candidate_storage.subspan(scalar_offset + index * new_stride, capacity);
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    for (std::size_t index = 0; index < FieldCount; ++index) {
        std::copy(fields_[index].begin(), fields_[index].end(), candidate_fields[index].begin());
    }

    storage_ = std::move(candidate);
    fields_ = candidate_fields;
    stride_ = new_stride;
    count_ = capacity;

    return BLITZAR_STATUS_OK;
}

bool ParticleArena::IsValid() const noexcept
{
    return std::all_of(fields_.begin(), fields_.end(),
        [this](const auto field) { return field.size() == count_; });
}

std::span<blitzar_core::Scalar> ParticleArena::Mutable(Field field) noexcept
{
    return fields_[static_cast<std::size_t>(field)];
}

#define BLITZAR_PARTICLE_ARENA_VIEW(name) \
    std::span<blitzar_core::Scalar> ParticleArena::name() noexcept \
    { \
        return Mutable(Field::name); \
    }

BLITZAR_PARTICLE_ARENA_VIEW(PositionX)
BLITZAR_PARTICLE_ARENA_VIEW(PositionY)
BLITZAR_PARTICLE_ARENA_VIEW(PositionZ)
BLITZAR_PARTICLE_ARENA_VIEW(VelocityX)
BLITZAR_PARTICLE_ARENA_VIEW(VelocityY)
BLITZAR_PARTICLE_ARENA_VIEW(VelocityZ)
BLITZAR_PARTICLE_ARENA_VIEW(Mass)
BLITZAR_PARTICLE_ARENA_VIEW(AccelerationX)
BLITZAR_PARTICLE_ARENA_VIEW(AccelerationY)
BLITZAR_PARTICLE_ARENA_VIEW(AccelerationZ)
BLITZAR_PARTICLE_ARENA_VIEW(CheckpointPositionX)
BLITZAR_PARTICLE_ARENA_VIEW(CheckpointPositionY)
BLITZAR_PARTICLE_ARENA_VIEW(CheckpointPositionZ)
BLITZAR_PARTICLE_ARENA_VIEW(CheckpointVelocityX)
BLITZAR_PARTICLE_ARENA_VIEW(CheckpointVelocityY)
BLITZAR_PARTICLE_ARENA_VIEW(CheckpointVelocityZ)

#undef BLITZAR_PARTICLE_ARENA_VIEW

} // namespace blitzar_particles
