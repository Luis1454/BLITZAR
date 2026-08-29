#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_HILBERT_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_HILBERT_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_neighborhood {

class HilbertIndex final {
public:
    explicit HilbertIndex(NeighborParameters parameters);

    [[nodiscard]] bool Build(const NeighborFrame& frame);
    [[nodiscard]] NeighborSet Query(const NeighborFrame& frame) const;
    [[nodiscard]] std::size_t MemoryBytes() const noexcept;

private:
    struct Entry final {
        std::uint64_t key{};
        std::size_t particle{};
    };

    NeighborParameters parameters_{};
    std::vector<Entry> entries_{};
};

} // namespace blitzar_neighborhood

#endif
