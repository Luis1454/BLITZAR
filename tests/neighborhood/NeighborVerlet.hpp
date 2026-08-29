#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_VERLET_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_VERLET_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstddef>
#include <vector>

namespace blitzar_neighborhood {

class NeighborVerlet final {
public:
    explicit NeighborVerlet(NeighborParameters parameters);

    [[nodiscard]] bool Build(const NeighborFrame& frame);
    [[nodiscard]] bool NeedsRebuild(const NeighborFrame& frame) const noexcept;
    [[nodiscard]] NeighborSet Query(const NeighborFrame& frame) const;
    [[nodiscard]] std::size_t MemoryBytes() const noexcept;

private:
    NeighborParameters parameters_{};
    std::vector<blitzar_core::Vector3> reference_positions_{};
    NeighborSet candidates_{};
};

} // namespace blitzar_neighborhood

#endif
