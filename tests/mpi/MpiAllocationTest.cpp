#include "MpiCases.hpp"
#include "fixtures/FixtureAllocationMonitor.hpp"
#include "fixtures/FixtureViews.hpp"

#include <array>

namespace blitzar_mpi_tests {

namespace {

bool CheckSteadyState(blitzar_sim::Sim& simulation) noexcept
{
    StateArrays output{};

    blitzar_tests::BeginAllocationCounting();

    const blitzar_status step_status = simulation.Step();
    const blitzar_status state_status = simulation.GetState(blitzar_tests::MakeOutputView(output));

    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    return step_status == BLITZAR_STATUS_OK && state_status == BLITZAR_STATUS_OK &&
           allocations == 0;
}

bool Warmup(blitzar_sim::Sim& simulation) noexcept
{
    StateArrays warmup{};

    return simulation.Step() == BLITZAR_STATUS_OK &&
           simulation.GetState(blitzar_tests::MakeOutputView(warmup)) == BLITZAR_STATUS_OK;
}

} // namespace

bool RunAllocationCase() noexcept
{
    const StateArrays initial = InitialState();
    const std::array<blitzar_solver_kind, 3> solvers{
        BLITZAR_SOLVER_DIRECT, BLITZAR_SOLVER_BARNES_HUT, BLITZAR_SOLVER_FMM};

    for (const blitzar_solver_kind solver_kind : solvers) {
        blitzar_sim::Sim simulation(ParticleCount);

        if (!Configure(simulation, initial, 0.01, solver_kind) || !Warmup(simulation) ||
            !CheckSteadyState(simulation)) {
            return false;
        }
    }

    return true;
}

bool RunMigrationAllocationCase() noexcept
{
    const StateArrays initial = MigrationState();
    blitzar_sim::Sim simulation(ParticleCount);

    if (!Configure(simulation, initial, 0.01) || !Warmup(simulation)) {
        return false;
    }

    return CheckSteadyState(simulation);
}

bool RunDistributedMeshRejectionCase() noexcept
{
    blitzar_parallel::MpiContext context;

    if (!context.IsUsable() || context.Size() <= 1) {
        return true;
    }

    const StateArrays initial = InitialState();
    const std::array<blitzar_solver_kind, 2> solvers{BLITZAR_SOLVER_PM, BLITZAR_SOLVER_TREEPM};

    for (const blitzar_solver_kind solver_kind : solvers) {
        blitzar_sim::Sim simulation(ParticleCount);

        if (!Configure(simulation, initial, 0.01, solver_kind)) {
            return false;
        }

        StateArrays before{};
        StateArrays after{};

        if (simulation.GetState(blitzar_tests::MakeOutputView(before)) != BLITZAR_STATUS_OK ||
            simulation.Step() != BLITZAR_STATUS_UNSUPPORTED ||
            simulation.GetState(blitzar_tests::MakeOutputView(after)) != BLITZAR_STATUS_OK ||
            !StatesMatch(before, after)) {
            return false;
        }
    }

    return true;
}

} // namespace blitzar_mpi_tests
