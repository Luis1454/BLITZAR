#ifndef BLITZAR_TESTS_SCALING_SCALING_HPP
#define BLITZAR_TESTS_SCALING_SCALING_HPP

#include "mpi/exchange/MpiExchangeTrace.hpp"
#include "simulation/Sim.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace blitzar_scaling {

enum class SolverKind : std::uint8_t { Direct, BarnesHut, Fmm };
enum class OverlapMode : std::uint8_t { Overlapped, Serialized };
enum class DistributionKind : std::uint8_t { BoxPair, BoundaryCrossing };

struct Config final {
    std::size_t particle_count{16};
    int warmup_steps{1};
    int timed_steps{3};
    std::uint64_t seed{424242};
    double oracle_tolerance{0.05};
    SolverKind solver{SolverKind::Direct};
    OverlapMode overlap{OverlapMode::Overlapped};
    DistributionKind distribution{DistributionKind::BoxPair};
    bool oracle{false};
    bool migration{false};
};

struct State final {
    explicit State(std::size_t count);

    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;
    std::vector<double> velocity_x;
    std::vector<double> velocity_y;
    std::vector<double> velocity_z;
    std::vector<double> mass;
};

struct Result final {
    blitzar_status status{BLITZAR_STATUS_INTERNAL_ERROR};
    blitzar_backend_kind backend{BLITZAR_BACKEND_CPU};
    int rank{};
    int ranks{1};
    std::size_t local_before{};
    std::size_t local_after{};
    std::uint64_t elapsed_ns{};
    std::uint64_t mean_step_ns{};
    std::uint64_t min_step_ns{};
    std::uint64_t max_step_ns{};
    std::size_t allocation_count{};
    std::uint64_t peak_rss_bytes{};
    double throughput_particles_per_second{};
    double oracle_max_error{};
    bool oracle_checked{};
    bool oracle_pass{};
    blitzar_parallel::MpiOverlapTrace overlap_trace{};
    blitzar_parallel::MpiMigrationTrace migration_trace{};
};

[[nodiscard]] State MakeState(std::size_t count, std::uint64_t seed, DistributionKind distribution);
[[nodiscard]] blitzar_core::ParticleStateView InputView(const State& state) noexcept;
[[nodiscard]] blitzar_core::ParticleOutputView OutputView(State& state) noexcept;
[[nodiscard]] bool Configure(
    blitzar_sim::Sim& simulation, const Config& config, const State& input) noexcept;
[[nodiscard]] std::string_view SolverName(SolverKind solver) noexcept;
[[nodiscard]] std::string_view OverlapName(OverlapMode mode) noexcept;
[[nodiscard]] std::string_view DistributionName(DistributionKind distribution) noexcept;
[[nodiscard]] blitzar_solver_kind PublicSolver(SolverKind solver) noexcept;
[[nodiscard]] bool Run(const Config& config, Result& result);

} // namespace blitzar_scaling

#endif
