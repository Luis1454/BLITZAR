#ifndef BLITZAR_TESTS_BVH_BVH_BENCHMARK_HPP
#define BLITZAR_TESTS_BVH_BVH_BENCHMARK_HPP

#include "bvh/BvhModel.hpp"

#include <cstdint>
#include <vector>

namespace blitzar_bvh {

class BvhBenchmark final {
public:
    explicit BvhBenchmark(std::uint64_t seed);

    [[nodiscard]] bool Run(std::vector<BvhScenarioResult>& results) const;

private:
    std::uint64_t seed_{};
};

} // namespace blitzar_bvh

#endif
