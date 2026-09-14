#ifndef BLITZAR_PHYSICS_NEIGHBORS_NEIGHBOR_INDEX_HPP
#define BLITZAR_PHYSICS_NEIGHBORS_NEIGHBOR_INDEX_HPP

#include "core/CoreTypes.hpp"

#include <blitzar/c/blitzar.h>
#include <cstddef>
#include <span>
#include <vector>

namespace blitzar_physics {

struct NeighborParameters final {
    blitzar_core::Scalar radius{};
    blitzar_core::Scalar skin{};
    std::size_t capacity{};
    std::size_t max_neighbors{};
    blitzar_core::Vector3 box_min{};
    blitzar_core::Vector3 box_max{};
};

struct NeighborSpan final {
    std::span<const std::size_t> indices{};
};

class NeighborIndex final {
public:
    explicit NeighborIndex(NeighborParameters parameters) noexcept;

    [[nodiscard]] bool IsValid() const noexcept
    {
        return valid_;
    }

    [[nodiscard]] bool IsBuilt() const noexcept
    {
        return built_;
    }

    [[nodiscard]] blitzar_status Build(std::span<const blitzar_core::Scalar> x,
        std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z) noexcept;

    [[nodiscard]] NeighborSpan Query(std::size_t target) const noexcept
    {
        if (!built_ || target >= built_count_) {
            return {std::span<const std::size_t>{}};
        }

        const std::size_t begin = neighbor_offsets_[target];
        const std::size_t end = neighbor_offsets_[target + 1];

        return {std::span<const std::size_t>(neighbors_.data() + begin, end - begin)};
    }

    [[nodiscard]] std::size_t Count() const noexcept
    {
        return built_count_;
    }

    [[nodiscard]] std::size_t CellCount() const noexcept
    {
        return cell_count_;
    }

    [[nodiscard]] std::size_t MemoryBytes() const noexcept
    {
        return cell_counts_.size() * sizeof(std::size_t) +
               cell_offsets_.size() * sizeof(std::size_t) + entries_.size() * sizeof(std::size_t) +
               neighbors_.size() * sizeof(std::size_t) +
               neighbor_offsets_.size() * sizeof(std::size_t);
    }

private:
    void BuildDimensions() noexcept;
    void WriteEntries(std::span<const blitzar_core::Scalar> x,
        std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z,
        std::size_t count) noexcept;
    [[nodiscard]] bool PreflightNeighborCounts(std::span<const blitzar_core::Scalar> x,
        std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z,
        std::size_t count) noexcept;
    void FillNeighbors(std::span<const blitzar_core::Scalar> x,
        std::span<const blitzar_core::Scalar> y, std::span<const blitzar_core::Scalar> z,
        std::size_t count) noexcept;
    [[nodiscard]] std::size_t CellIndex(
        blitzar_core::Scalar px, blitzar_core::Scalar py, blitzar_core::Scalar pz) const noexcept;

    NeighborParameters parameters_{};
    std::size_t cell_count_{};
    std::size_t built_count_{};
    std::size_t neighbour_span_x_{1};
    std::size_t neighbour_span_y_{1};
    std::size_t neighbour_span_z_{1};
    std::vector<std::size_t> cell_counts_{};
    std::vector<std::size_t> cell_offsets_{};
    std::vector<std::size_t> entries_{};
    std::vector<std::size_t> neighbors_{};
    std::vector<std::size_t> neighbor_offsets_{};
    bool valid_{false};
    bool built_{false};
};

} // namespace blitzar_physics

#endif
