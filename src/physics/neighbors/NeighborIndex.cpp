#include "physics/neighbors/NeighborIndex.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

bool IsFinite(blitzar_core::Vector3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool ParametersValid(blitzar_physics::NeighborParameters parameters)
{
    if (!(parameters.radius > 0.0) || !std::isfinite(parameters.radius)) {
        return false;
    }

    if (parameters.skin < 0.0 || !std::isfinite(parameters.skin)) {
        return false;
    }

    if (parameters.capacity == 0) {
        return false;
    }

    if (parameters.max_neighbors == 0) {
        return false;
    }

    if (!IsFinite(parameters.box_min) || !IsFinite(parameters.box_max)) {
        return false;
    }

    if (parameters.box_min.x >= parameters.box_max.x ||
        parameters.box_min.y >= parameters.box_max.y ||
        parameters.box_min.z >= parameters.box_max.z) {
        return false;
    }

    const std::size_t max = std::numeric_limits<std::size_t>::max();

    if (parameters.capacity > max / parameters.max_neighbors) {
        return false;
    }

    return true;
}

} // namespace

namespace blitzar_physics {

NeighborIndex::NeighborIndex(NeighborParameters parameters) noexcept : parameters_(parameters)
{
    valid_ = ParametersValid(parameters_);

    if (!valid_) {
        return;
    }

    BuildDimensions();

    if (!valid_) {
        return;
    }

    cell_counts_.resize(cell_count_);
    cell_offsets_.resize(cell_count_);
    entries_.resize(parameters_.capacity);
    neighbors_.resize(parameters_.capacity * parameters_.max_neighbors);
    neighbor_offsets_.resize(parameters_.capacity + 1);
}

void NeighborIndex::BuildDimensions() noexcept
{
    const blitzar_core::Scalar cell_size = parameters_.radius + parameters_.skin;
    const blitzar_core::Scalar extent_x = parameters_.box_max.x - parameters_.box_min.x;
    const blitzar_core::Scalar extent_y = parameters_.box_max.y - parameters_.box_min.y;
    const blitzar_core::Scalar extent_z = parameters_.box_max.z - parameters_.box_min.z;

    neighbour_span_x_ = static_cast<std::size_t>(std::floor(extent_x / cell_size)) + 1;
    neighbour_span_y_ = static_cast<std::size_t>(std::floor(extent_y / cell_size)) + 1;
    neighbour_span_z_ = static_cast<std::size_t>(std::floor(extent_z / cell_size)) + 1;

    const std::size_t max = std::numeric_limits<std::size_t>::max();

    if (neighbour_span_x_ > max / neighbour_span_y_ ||
        neighbour_span_y_ > max / neighbour_span_z_) {
        valid_ = false;

        return;
    }

    cell_count_ = neighbour_span_x_ * neighbour_span_y_ * neighbour_span_z_;

    if (cell_count_ > parameters_.capacity * parameters_.capacity) {
        valid_ = false;
    }
}

std::size_t NeighborIndex::CellIndex(
    blitzar_core::Scalar px, blitzar_core::Scalar py, blitzar_core::Scalar pz) const noexcept
{
    const blitzar_core::Scalar cell_size = parameters_.radius + parameters_.skin;

    const long long cx =
        std::clamp(static_cast<long long>(std::floor((px - parameters_.box_min.x) / cell_size)),
            0LL, static_cast<long long>(neighbour_span_x_) - 1LL);

    const long long cy =
        std::clamp(static_cast<long long>(std::floor((py - parameters_.box_min.y) / cell_size)),
            0LL, static_cast<long long>(neighbour_span_y_) - 1LL);

    const long long cz =
        std::clamp(static_cast<long long>(std::floor((pz - parameters_.box_min.z) / cell_size)),
            0LL, static_cast<long long>(neighbour_span_z_) - 1LL);

    return static_cast<std::size_t>(cx) + static_cast<std::size_t>(cy) * neighbour_span_x_ +
           static_cast<std::size_t>(cz) * neighbour_span_x_ * neighbour_span_y_;
}

bool NeighborIndex::PreflightNeighborCounts(std::span<const blitzar_core::Scalar> x,
    std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z,
    std::size_t count) noexcept
{
    const blitzar_core::Scalar radius_squared = parameters_.radius * parameters_.radius;

    for (std::size_t target = 0; target < count; ++target) {
        std::size_t neighbors = 0;

        for (std::size_t source = 0; source < count; ++source) {
            if (source == target) {
                continue;
            }

            const blitzar_core::Scalar dx = x[target] - x[source];
            const blitzar_core::Scalar dy = y[target] - y[source];
            const blitzar_core::Scalar dz = z[target] - z[source];

            if (dx * dx + dy * dy + dz * dz <= radius_squared) {
                ++neighbors;
            }
        }

        if (neighbors > parameters_.max_neighbors) {
            return false;
        }
    }

    return true;
}

void NeighborIndex::WriteEntries(std::span<const blitzar_core::Scalar> x,
    std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z,
    std::size_t count) noexcept
{
    std::fill(cell_counts_.begin(), cell_counts_.end(), 0);

    for (std::size_t index = 0; index < count; ++index) {
        const std::size_t cell = CellIndex(x[index], y[index], z[index]);

        entries_[cell_offsets_[cell] + cell_counts_[cell]] = index;

        ++cell_counts_[cell];
    }
}

void NeighborIndex::FillNeighbors(std::span<const blitzar_core::Scalar> x,
    std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z,
    std::size_t count) noexcept
{
    const blitzar_core::Scalar radius_squared = parameters_.radius * parameters_.radius;
    std::size_t cursor = 0;

    neighbor_offsets_[0] = 0;

    for (std::size_t target = 0; target < count; ++target) {
        const std::size_t begin = cursor;
        const std::size_t target_cell = CellIndex(x[target], y[target], z[target]);

        const long long tx = static_cast<long long>(target_cell % neighbour_span_x_);
        const long long ty =
            static_cast<long long>((target_cell / neighbour_span_x_) % neighbour_span_y_);

        const long long tz =
            static_cast<long long>(target_cell / (neighbour_span_x_ * neighbour_span_y_));

        for (long long dz = -1; dz <= 1; ++dz) {
            for (long long dy = -1; dy <= 1; ++dy) {
                for (long long dx = -1; dx <= 1; ++dx) {
                    const std::size_t cx = static_cast<std::size_t>(
                        std::clamp(tx + dx, 0LL, static_cast<long long>(neighbour_span_x_) - 1LL));

                    const std::size_t cy = static_cast<std::size_t>(
                        std::clamp(ty + dy, 0LL, static_cast<long long>(neighbour_span_y_) - 1LL));

                    const std::size_t cz = static_cast<std::size_t>(
                        std::clamp(tz + dz, 0LL, static_cast<long long>(neighbour_span_z_) - 1LL));

                    const std::size_t cell =
                        cx + cy * neighbour_span_x_ + cz * neighbour_span_x_ * neighbour_span_y_;

                    const std::size_t cell_begin = cell_offsets_[cell];

                    for (std::size_t slot = 0; slot < cell_counts_[cell]; ++slot) {
                        const std::size_t source = entries_[cell_begin + slot];

                        if (source == target) {
                            continue;
                        }

                        const blitzar_core::Scalar dx = x[target] - x[source];
                        const blitzar_core::Scalar dy = y[target] - y[source];
                        const blitzar_core::Scalar dz = z[target] - z[source];

                        if (dx * dx + dy * dy + dz * dz <= radius_squared) {
                            neighbors_[cursor] = source;

                            ++cursor;
                        }
                    }
                }
            }
        }

        std::sort(neighbors_.begin() + static_cast<std::ptrdiff_t>(begin),
            neighbors_.begin() + static_cast<std::ptrdiff_t>(cursor));

        neighbor_offsets_[target + 1] = cursor;
    }
}

blitzar_status NeighborIndex::Build(std::span<const blitzar_core::Scalar> x,
    std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z) noexcept
{
    if (!valid_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (x.size() != y.size() || x.size() != z.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t count = x.size();

    if (count != 0 && count > parameters_.capacity) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    built_ = false;
    built_count_ = 0;

    std::fill(neighbor_offsets_.begin(), neighbor_offsets_.end(), 0);

    if (count == 0) {
        built_ = true;

        return BLITZAR_STATUS_OK;
    }

    if (!PreflightNeighborCounts(x, y, z, count)) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    std::fill(cell_counts_.begin(), cell_counts_.end(), 0);

    for (std::size_t index = 0; index < count; ++index) {
        std::size_t cell = 0;

        if (!std::isfinite(x[index]) || !std::isfinite(y[index]) || !std::isfinite(z[index])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        cell = CellIndex(x[index], y[index], z[index]);

        ++cell_counts_[cell];
    }

    std::size_t running = 0;

    for (std::size_t cell = 0; cell < cell_count_; ++cell) {
        cell_offsets_[cell] = running;
        running += cell_counts_[cell];
    }

    WriteEntries(x, y, z, count);
    FillNeighbors(x, y, z, count);

    built_ = true;
    built_count_ = count;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_physics
