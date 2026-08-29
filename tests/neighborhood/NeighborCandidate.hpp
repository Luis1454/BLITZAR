#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_CANDIDATE_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_CANDIDATE_HPP

#include "neighborhood/NeighborGrid.hpp"
#include "neighborhood/NeighborHilbert.hpp"
#include "neighborhood/NeighborModel.hpp"
#include "neighborhood/NeighborVerlet.hpp"

#include <cstddef>
#include <variant>

namespace blitzar_neighborhood {

class NeighborCandidate final {
public:
    NeighborCandidate(const NeighborWorkload& workload, CandidateKind candidate);

    [[nodiscard]] bool NeedsRebuild(const NeighborFrame& frame) const noexcept;
    [[nodiscard]] bool Build(const NeighborFrame& frame);
    [[nodiscard]] NeighborSet Query(const NeighborFrame& frame) const;
    [[nodiscard]] std::size_t MemoryBytes() const noexcept;

private:
    using Index = std::variant<GridIndex, HilbertIndex, NeighborVerlet>;

    [[nodiscard]] static Index MakeIndex(const NeighborWorkload& workload, CandidateKind candidate);

    CandidateKind kind_{};
    Index index_;
};

} // namespace blitzar_neighborhood

#endif
