#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_BENCHMARK_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_BENCHMARK_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstdint>
#include <vector>

namespace blitzar_neighborhood {

class NeighborBenchmark final {
public:
    explicit NeighborBenchmark(std::uint64_t seed);

    [[nodiscard]] bool Run(std::vector<NeighborResult>& results) const;

private:
    std::uint64_t seed_{};
};

} // namespace blitzar_neighborhood

#endif
