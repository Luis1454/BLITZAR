#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_GRID_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_GRID_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_neighborhood {

struct GridCell final {
    int x{};
    int y{};
    int z{};
};

struct GridEntry final {
    GridCell cell{};
    std::uint64_t key{};
    std::size_t particle{};
};

struct GridRange final {
    GridCell cell{};
    std::uint64_t key{};
    std::size_t begin{};
    std::size_t end{};
};

class GridIndex final {
public:
    GridIndex(NeighborParameters parameters, GridKind kind);

    [[nodiscard]] bool Build(const NeighborFrame& frame);
    [[nodiscard]] NeighborSet Query(const NeighborFrame& frame) const;
    [[nodiscard]] std::size_t MemoryBytes() const noexcept;

private:
    [[nodiscard]] GridCell Locate(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] std::uint64_t Key(GridCell cell) const noexcept;
    [[nodiscard]] std::size_t RangeIndex(GridCell cell) const noexcept;
    void BuildRanges();

    NeighborParameters parameters_{};
    GridKind kind_{};
    GridCell dimensions_{};
    std::vector<GridEntry> entries_{};
    std::vector<GridRange> ranges_{};
};

} // namespace blitzar_neighborhood

#endif
