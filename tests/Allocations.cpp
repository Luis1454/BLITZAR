#include "AllocationMonitor.hpp"
#include "Check.hpp"
#include "parallel/MpiContext.hpp"
#include "sdk/Simulation.hpp"

#include <array>
#include <cstddef>

namespace {

constexpr std::size_t ParticleCount = 2;

struct StateArrays final {
    std::array<double, ParticleCount> position_x{0.0, 1.0};
    std::array<double, ParticleCount> position_y{0.0, 0.0};
    std::array<double, ParticleCount> position_z{0.0, 0.0};
    std::array<double, ParticleCount> velocity_x{0.0, 0.0};
    std::array<double, ParticleCount> velocity_y{0.0, 0.0};
    std::array<double, ParticleCount> velocity_z{0.0, 0.0};
    std::array<double, ParticleCount> mass{1.0, 1.0};
};

[[nodiscard]] blitzar_core::ParticleOutputView MakeOutputView(StateArrays& state) noexcept
{
    return {state.position_x.size(), state.position_x, state.position_y, state.position_z,
        state.velocity_x, state.velocity_y, state.velocity_z, state.mass};
}

[[nodiscard]] bool Configure(blitzar_sdk::Simulation& simulation, const StateArrays& state,
    blitzar_solver_kind solver) noexcept
{
    if (solver == BLITZAR_SOLVER_BARNES_HUT &&
        simulation.SetBarnesHut({0.0, ParticleCount, 128, 1, 32}) != BLITZAR_STATUS_OK) {
        return false;
    }

    return simulation.SetSolver(solver) == BLITZAR_STATUS_OK &&
           simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(0.01) == BLITZAR_STATUS_OK &&
           simulation.SetParticles({state.position_x.size(), state.position_x, state.position_y,
               state.position_z, state.velocity_x, state.velocity_y, state.velocity_z, state.mass,
               state.position_x.size()}) == BLITZAR_STATUS_OK;
}

[[nodiscard]] bool RunCase(blitzar_sdk::Simulation& simulation, blitzar_solver_kind solver) noexcept
{
    const StateArrays state{};

    if (!Configure(simulation, state, solver)) {
        return false;
    }

    StateArrays warmup{};

    if (simulation.Step() != BLITZAR_STATUS_OK ||
        simulation.GetState(MakeOutputView(warmup)) != BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_tests::BeginAllocationCounting();

    const blitzar_status first_step = simulation.Step();
    const blitzar_status second_step = simulation.Step();
    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    return first_step == BLITZAR_STATUS_OK && second_step == BLITZAR_STATUS_OK && allocations == 0;
}

[[nodiscard]] bool RunSequentialContextCase() noexcept
{
    {
        blitzar_parallel::MpiContext first;

        if (!first.IsUsable()) {
            return false;
        }
    }

    blitzar_parallel::MpiContext second;

    return second.IsUsable();
}

} // namespace

int main()
{
    {
        blitzar_sdk::Simulation simulation(ParticleCount);

        BLITZAR_CHECK(RunCase(simulation, BLITZAR_SOLVER_DIRECT));
        BLITZAR_CHECK(RunCase(simulation, BLITZAR_SOLVER_BARNES_HUT));
    }

    BLITZAR_CHECK(RunSequentialContextCase());

    return 0;
}
