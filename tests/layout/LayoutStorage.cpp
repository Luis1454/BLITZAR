#include "layout/LayoutStorage.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <limits>

namespace blitzar_layout {

LayoutStorage::LayoutStorage(std::size_t count, LayoutKind kind, std::size_t tile_width)
    : count_(count), kind_(kind), tile_width_(tile_width)
{
    for (auto& field : fields_) {
        field.resize(count_);
    }

    if (kind_ != LayoutKind::Aosoa || tile_width_ == 0 ||
        count_ > (std::numeric_limits<std::size_t>::max() - tile_width_ + 1U) / tile_width_) {
        return;
    }

    const std::size_t tile_count = count_ == 0 ? 0 : 1 + (count_ - 1) / tile_width_;

    tiled_.resize(tile_count * FieldCount * tile_width_);
}

bool LayoutStorage::IsValid() const noexcept
{
    if (kind_ == LayoutKind::Soa) {
        for (const auto& field : fields_) {
            if (field.size() != count_) {
                return false;
            }
        }

        return tile_width_ == 0 && tiled_.empty();
    }

    if (tile_width_ == 0 || (tiled_.empty() && count_ != 0)) {
        return false;
    }

    for (const auto& field : fields_) {
        if (field.size() != count_) {
            return false;
        }
    }

    return tiled_.size() >= count_ * FieldCount;
}

bool LayoutStorage::Load(const LayoutState& state, std::span<const std::size_t> order) noexcept
{
    if (!IsValid() || state.Count() != count_ || order.size() != count_) {
        return false;
    }

    const auto source = state.View();

    for (std::size_t destination = 0; destination < count_; ++destination) {
        const std::size_t source_index = order[destination];

        if (source_index >= source.SourceCount()) {
            return false;
        }

        const std::array<blitzar_core::Scalar, FieldCount> values{source.x[source_index],
            source.y[source_index], source.z[source_index], source.velocity_x[source_index],
            source.velocity_y[source_index], source.velocity_z[source_index],
            source.mass[source_index]};

        for (std::size_t field = 0; field < FieldCount; ++field) {
            fields_[field][destination] = values[field];

            StoreCandidate(field, destination, values[field]);
        }
    }

    return true;
}

blitzar_core::ParticleStateView LayoutStorage::View() const noexcept
{
    return {count_, std::span<const blitzar_core::Scalar>(fields_[0]),
        std::span<const blitzar_core::Scalar>(fields_[1]),
        std::span<const blitzar_core::Scalar>(fields_[2]),
        std::span<const blitzar_core::Scalar>(fields_[3]),
        std::span<const blitzar_core::Scalar>(fields_[4]),
        std::span<const blitzar_core::Scalar>(fields_[5]),
        std::span<const blitzar_core::Scalar>(fields_[6])};
}

double LayoutStorage::Scan() const noexcept
{
    double total = 0.0;

    for (std::size_t field = 0; field < FieldCount; ++field) {
#if defined(_OPENMP)
#pragma omp simd reduction(+ : total)
#endif
        for (std::size_t index = 0; index < count_; ++index) {
            total += CandidateValue(field, index);
        }
    }

    return total;
}

std::size_t LayoutStorage::CandidateBytes() const noexcept
{
    if (kind_ == LayoutKind::Aosoa) {
        return tiled_.size() * sizeof(blitzar_core::Scalar);
    }

    return count_ * FieldCount * sizeof(blitzar_core::Scalar);
}

std::size_t LayoutStorage::MaterializedBytes() const noexcept
{
    return kind_ == LayoutKind::Aosoa ? count_ * FieldCount * sizeof(blitzar_core::Scalar) : 0;
}

std::size_t LayoutStorage::CacheLineVisits() const noexcept
{
    if (kind_ == LayoutKind::Aosoa) {
        const std::size_t bytes = CandidateBytes();

        return bytes == 0 ? 0 : 1 + (bytes - 1) / CacheLineBytes;
    }

    std::size_t lines = 0;

    for (const auto& field : fields_) {
        const std::size_t bytes = field.size() * sizeof(blitzar_core::Scalar);

        lines += bytes == 0 ? 0 : 1 + (bytes - 1) / CacheLineBytes;
    }

    return lines;
}

std::uint64_t LayoutStorage::LogicalHash() const noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    for (std::size_t index = 0; index < count_; ++index) {
        for (std::size_t field = 0; field < FieldCount; ++field) {
            hash ^= std::bit_cast<std::uint64_t>(CandidateValue(field, index));
            hash *= 1099511628211ULL;
        }
    }

    return hash;
}

std::uint64_t LayoutStorage::ByteHash() const noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    const auto append = [&hash](const blitzar_core::Scalar value) noexcept {
        hash ^= std::bit_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };

    if (kind_ == LayoutKind::Aosoa) {
        for (const auto value : tiled_) {
            append(value);
        }

        return hash;
    }

    for (const auto& field : fields_) {
        for (const auto value : field) {
            append(value);
        }
    }

    return hash;
}

blitzar_core::Scalar LayoutStorage::CandidateValue(
    std::size_t field, std::size_t index) const noexcept
{
    if (kind_ == LayoutKind::Soa) {
        return fields_[field][index];
    }

    const std::size_t tile = index / tile_width_;
    const std::size_t lane = index % tile_width_;

    return tiled_[(tile * FieldCount + field) * tile_width_ + lane];
}

void LayoutStorage::StoreCandidate(
    std::size_t field, std::size_t index, blitzar_core::Scalar value) noexcept
{
    if (kind_ == LayoutKind::Soa) {
        return;
    }

    const std::size_t tile = index / tile_width_;
    const std::size_t lane = index % tile_width_;

    tiled_[(tile * FieldCount + field) * tile_width_ + lane] = value;
}

} // namespace blitzar_layout
