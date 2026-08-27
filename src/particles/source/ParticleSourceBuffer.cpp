#include "particles/source/ParticleSourceBuffer.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>

namespace blitzar_particles {

static_assert(64 % sizeof(blitzar_core::Scalar) == 0,
    "Source buffer alignment must be a multiple of the scalar size");

std::size_t ParticleSourceBuffer::AlignedCount(std::size_t count)
{
    const std::size_t remainder = count % ScalarsPerAlignment;
    const std::size_t padding = remainder == 0 ? 0 : ScalarsPerAlignment - remainder;

    if (count > std::numeric_limits<std::size_t>::max() - padding) {
        throw std::length_error("source buffer is too large");
    }

    return count + padding;
}

ParticleSourceBuffer::ParticleSourceBuffer(std::size_t capacity)
{
    if (capacity != 0) {
        const blitzar_status status = Reserve(capacity);

        if (status == BLITZAR_STATUS_ALLOCATION_FAILURE) {
            throw std::bad_alloc();
        }
        if (status != BLITZAR_STATUS_OK) {
            throw std::length_error("source buffer capacity is invalid");
        }
    }
}

blitzar_status ParticleSourceBuffer::Reserve(std::size_t capacity) noexcept
{
    if (capacity <= capacity_) {
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

    if (new_stride == 0 && capacity != 0) {
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
    capacity_ = capacity;

    return BLITZAR_STATUS_OK;
}

blitzar_status ParticleSourceBuffer::SetCount(std::size_t count) noexcept
{
    if (count > capacity_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    count_ = count;

    return BLITZAR_STATUS_OK;
}

std::size_t ParticleSourceBuffer::Count() const noexcept
{
    return count_;
}

std::size_t ParticleSourceBuffer::Capacity() const noexcept
{
    return capacity_;
}

bool ParticleSourceBuffer::IsValid() const noexcept
{
    return count_ <= capacity_ &&
           std::all_of(fields_.begin(), fields_.end(),
               [this](const auto field) { return field.size() == capacity_; });
}

blitzar_core::ParticleStateView ParticleSourceBuffer::State() const noexcept
{
    if (!IsValid()) {
        return {};
    }

    return {count_, fields_[static_cast<std::size_t>(Field::PositionX)].first(count_),
        fields_[static_cast<std::size_t>(Field::PositionY)].first(count_),
        fields_[static_cast<std::size_t>(Field::PositionZ)].first(count_),
        fields_[static_cast<std::size_t>(Field::VelocityX)].first(count_),
        fields_[static_cast<std::size_t>(Field::VelocityY)].first(count_),
        fields_[static_cast<std::size_t>(Field::VelocityZ)].first(count_),
        fields_[static_cast<std::size_t>(Field::Mass)].first(count_), count_};
}

std::span<blitzar_core::Scalar> ParticleSourceBuffer::Mutable(Field field) noexcept
{
    return fields_[static_cast<std::size_t>(field)];
}

#define BLITZAR_SOURCE_BUFFER_VIEW(name) \
    std::span<blitzar_core::Scalar> ParticleSourceBuffer::name() noexcept \
    { \
        return Mutable(Field::name); \
    }

BLITZAR_SOURCE_BUFFER_VIEW(PositionX)
BLITZAR_SOURCE_BUFFER_VIEW(PositionY)
BLITZAR_SOURCE_BUFFER_VIEW(PositionZ)
BLITZAR_SOURCE_BUFFER_VIEW(VelocityX)
BLITZAR_SOURCE_BUFFER_VIEW(VelocityY)
BLITZAR_SOURCE_BUFFER_VIEW(VelocityZ)
BLITZAR_SOURCE_BUFFER_VIEW(Mass)

#undef BLITZAR_SOURCE_BUFFER_VIEW

} // namespace blitzar_particles
