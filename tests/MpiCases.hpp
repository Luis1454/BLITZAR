#ifndef BLITZAR_TESTS_MPI_CASES_HPP
#define BLITZAR_TESTS_MPI_CASES_HPP

#include "parallel/MpiContext.hpp"
#include "sdk/Simulation.hpp"

#include <array>
#include <cstddef>

namespace blitzar_mpi_tests {

inline constexpr std::size_t ParticleCount = 8;

using StateArray = std::array<double, ParticleCount>;

struct StateArrays final {
    StateArray x{};
    StateArray y{};
    StateArray z{};
    StateArray velocity_x{};
    StateArray velocity_y{};
    StateArray velocity_z{};
    StateArray mass{};
};

[[nodiscard]] StateArrays InitialState() noexcept;
[[nodiscard]] StateArrays MigrationState() noexcept;

[[nodiscard]] bool Configure(blitzar_sdk::Simulation& simulation, const StateArrays& state,
    double timestep, blitzar_solver_kind solver_kind = BLITZAR_SOLVER_DIRECT) noexcept;
[[nodiscard]] bool BuildReference(
    const StateArrays& initial, StateArrays& result, double timestep, int step_count) noexcept;
[[nodiscard]] bool RunCase(const StateArrays& initial, double timestep, int step_count,
    blitzar_solver_kind solver_kind = BLITZAR_SOLVER_DIRECT) noexcept;
[[nodiscard]] bool StatesMatch(const StateArrays& left, const StateArrays& right) noexcept;

[[nodiscard]] bool RunBoundaryOwnershipCase(blitzar_parallel::MpiContext& context) noexcept;
[[nodiscard]] bool RunOverlapCase(blitzar_parallel::MpiContext& context) noexcept;
[[nodiscard]] bool RunAllocationCase() noexcept;
[[nodiscard]] bool RunMigrationAllocationCase() noexcept;
[[nodiscard]] bool RunRollbackCase() noexcept;
[[nodiscard]] bool RunOutOfDomainCase() noexcept;

[[nodiscard]] bool RunErrorSynchronizationCase(blitzar_parallel::MpiContext& context) noexcept;
[[nodiscard]] bool RunNestedContextCase(const blitzar_parallel::MpiContext& context) noexcept;
[[nodiscard]] bool RunCollectiveValidationCase(
    const blitzar_parallel::MpiContext& context) noexcept;
[[nodiscard]] bool RunLargeCountValidationCase(
    const blitzar_parallel::MpiContext& context) noexcept;
[[nodiscard]] bool RunWireCodecCase() noexcept;

} // namespace blitzar_mpi_tests

#endif
