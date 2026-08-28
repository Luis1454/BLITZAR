#include "ScaleTest.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace blitzar_scaling {

namespace {

constexpr std::uint64_t Mix(std::uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;

    return value ^ (value >> 31U);
}

double Unit(std::uint64_t value) noexcept
{
    return static_cast<double>(value >> 11U) / 9007199254740992.0;
}

} // namespace

State::State(std::size_t count)
    : x(count), y(count), z(count), velocity_x(count), velocity_y(count), velocity_z(count),
      mass(count)
{
}

State MakeState(std::size_t count, std::uint64_t seed, DistributionKind distribution)
{
    State state(count);

    for (std::size_t index = 0; index < count; ++index) {
        const bool even = index % 2 == 0;

        if (distribution == DistributionKind::BoundaryCrossing) {
            state.x[index] = index == 0 ? -1.0 : index == 1 ? 1.0 : even ? -0.02 : 0.02;
            state.y[index] = 0.0;
            state.z[index] = 0.0;
            state.velocity_x[index] = index < 2 ? 0.0 : even ? 50.0 : -50.0;
            state.velocity_y[index] = 0.0;
            state.velocity_z[index] = 0.0;
        }
        else {
            state.x[index] = index == 0 ? -4.0
                             : index == 1
                                 ? 4.0
                                 : -3.0 + 6.0 * Unit(Mix(seed + static_cast<std::uint64_t>(index) +
                                                         0x123456789abcdef0ULL));

            state.y[index] = index == 0 ? -4.0
                             : index == 1
                                 ? 4.0
                                 : -3.0 + 6.0 * Unit(Mix(seed + static_cast<std::uint64_t>(index) +
                                                         0x0fedcba987654321ULL));

            state.z[index] = index == 0 ? -4.0
                             : index == 1
                                 ? 4.0
                                 : -3.0 + 6.0 * Unit(Mix(seed + static_cast<std::uint64_t>(index) +
                                                         0x3141592653589793ULL));

            state.velocity_x[index] = index < 2 ? 0.0 : 0.01 * (Unit(Mix(seed + index + 1U)) - 0.5);
            state.velocity_y[index] = index < 2 ? 0.0 : 0.01 * (Unit(Mix(seed + index + 2U)) - 0.5);
            state.velocity_z[index] = index < 2 ? 0.0 : 0.01 * (Unit(Mix(seed + index + 3U)) - 0.5);
        }

        state.mass[index] = 1.0 + 0.25 * static_cast<double>(index % 5);
    }

    return state;
}

blitzar_core::ParticleStateView InputView(const State& state) noexcept
{
    return {state.x.size(), state.x, state.y, state.z, state.velocity_x, state.velocity_y,
        state.velocity_z, state.mass, state.x.size()};
}

blitzar_core::ParticleOutputView OutputView(State& state) noexcept
{
    return {state.x.size(), state.x, state.y, state.z, state.velocity_x, state.velocity_y,
        state.velocity_z, state.mass};
}

bool Configure(blitzar_sim::Sim& simulation, const Config& config, const State& input) noexcept
{
    if (config.solver != SolverKind::Direct) {
        if (config.particle_count > (std::numeric_limits<std::size_t>::max() - 1U) / 8U) {
            return false;
        }

        const blitzar_barnes_hut::BarnesHutSettings settings{
            config.solver == SolverKind::BarnesHut ? 0.5 : 0.0, config.particle_count,
            config.particle_count * 8U + 1U, 8, 32};

        if (simulation.SetBarnesHut(settings) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    if (simulation.SetSolver(PublicSolver(config.solver)) != BLITZAR_STATUS_OK ||
        simulation.SetGravity(1.0, 0.05) != BLITZAR_STATUS_OK ||
        simulation.SetTimestep(0.001) != BLITZAR_STATUS_OK ||
        simulation.SetSeed(config.seed) != BLITZAR_STATUS_OK) {
        return false;
    }

    simulation.SetMpiOverlapForTesting(config.overlap == OverlapMode::Overlapped
                                           ? blitzar_parallel::MpiOverlapMode::Overlapped
                                           : blitzar_parallel::MpiOverlapMode::Serialized);

    return simulation.SetParticles(InputView(input)) == BLITZAR_STATUS_OK;
}

std::string_view SolverName(SolverKind solver) noexcept
{
    switch (solver) {
    case SolverKind::Direct:

        return "direct";

    case SolverKind::BarnesHut:

        return "barnes-hut";

    case SolverKind::Fmm:

        return "fmm";
    }

    return "unknown";
}

std::string_view OverlapName(OverlapMode mode) noexcept
{
    return mode == OverlapMode::Overlapped ? "overlapped" : "serialized";
}

std::string_view DistributionName(DistributionKind distribution) noexcept
{
    return distribution == DistributionKind::BoundaryCrossing ? "boundary-crossing-v1"
                                                              : "box-pair-v1";
}

blitzar_solver_kind PublicSolver(SolverKind solver) noexcept
{
    switch (solver) {
    case SolverKind::Direct:

        return BLITZAR_SOLVER_DIRECT;

    case SolverKind::BarnesHut:

        return BLITZAR_SOLVER_BARNES_HUT;

    case SolverKind::Fmm:

        return BLITZAR_SOLVER_FMM;
    }

    return BLITZAR_SOLVER_DIRECT;
}

} // namespace blitzar_scaling
