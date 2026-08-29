#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_CASE_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_CASE_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_neighborhood {

inline constexpr std::size_t kParticleCount = 96;
inline constexpr std::size_t kStepCount = 6;

[[nodiscard]] std::vector<NeighborWorkload> MakeWorkloads(std::uint64_t seed);
[[nodiscard]] NeighborFrame MakeFrame(const NeighborWorkload& workload, std::size_t step);
[[nodiscard]] bool IsMoving(ScenarioKind scenario) noexcept;

} // namespace blitzar_neighborhood

#endif
