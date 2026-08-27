#include "MpiCases.hpp"
#include "fixtures/FixtureAllocationMonitor.hpp"
#include "fixtures/FixtureViews.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

namespace blitzar_mpi_tests {

namespace {

StateArrays MakeRollbackState() noexcept
{
    StateArrays initial{};

    initial.x = {0.0, 1.0, 10.0, 11.0, 20.0, 21.0, 30.0, 31.0};

    initial.velocity_x.fill(0.0);

    for (std::size_t index = 1; index < ParticleCount; index += 2) {
        initial.velocity_x[index] = -1.0;
    }

    initial.velocity_y.fill(0.0);
    initial.velocity_z.fill(0.0);
    initial.mass.fill(1.0);

    return initial;
}

bool ConfigureRollback(blitzar_sim::Sim& simulation, const StateArrays& initial) noexcept
{
    return Configure(simulation, initial, 0.5) &&
           simulation.SetGravity(std::numeric_limits<double>::denorm_min(), 0.1) ==
               BLITZAR_STATUS_OK;
}

bool PrepareRollbackPair(
    blitzar_sim::Sim& simulation, blitzar_sim::Sim& expected, const StateArrays& initial) noexcept
{
    if (!ConfigureRollback(simulation, initial) || !ConfigureRollback(expected, initial)) {
        return false;
    }

    return simulation.Step() == BLITZAR_STATUS_OK && expected.Step() == BLITZAR_STATUS_OK;
}

bool TriggerRollbackFailure(blitzar_sim::Sim& simulation, StateArrays& before_failure) noexcept
{
    if (simulation.GetState(blitzar_tests::MakeOutputView(before_failure)) != BLITZAR_STATUS_OK ||
        simulation.SetGravity(std::numeric_limits<double>::denorm_min(), 0.0) !=
            BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_tests::BeginAllocationCounting();

    const blitzar_status failure_status = simulation.Step();
    const std::size_t failure_allocations = blitzar_tests::EndAllocationCounting();

    return failure_status == BLITZAR_STATUS_SINGULARITY && failure_allocations == 0;
}

bool StatesMatchAtTolerance(
    const StateArrays& left, const StateArrays& right, double tolerance) noexcept
{
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(left.x[index] - right.x[index]) > tolerance ||
            std::abs(left.y[index] - right.y[index]) > tolerance ||
            std::abs(left.z[index] - right.z[index]) > tolerance ||
            std::abs(left.velocity_x[index] - right.velocity_x[index]) > tolerance ||
            std::abs(left.velocity_y[index] - right.velocity_y[index]) > tolerance ||
            std::abs(left.velocity_z[index] - right.velocity_z[index]) > tolerance ||
            left.mass[index] != right.mass[index]) {
            return false;
        }
    }

    return true;
}

bool ValidateRollbackRetry(blitzar_sim::Sim& simulation, blitzar_sim::Sim& expected) noexcept
{
    if (simulation.SetGravity(std::numeric_limits<double>::denorm_min(), 0.1) !=
            BLITZAR_STATUS_OK ||
        expected.SetGravity(std::numeric_limits<double>::denorm_min(), 0.1) != BLITZAR_STATUS_OK) {
        return false;
    }

    if (simulation.Step() != BLITZAR_STATUS_OK || expected.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    StateArrays actual_retry{};
    StateArrays expected_retry{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(actual_retry)) != BLITZAR_STATUS_OK ||
        expected.GetState(blitzar_tests::MakeOutputView(expected_retry)) != BLITZAR_STATUS_OK) {
        return false;
    }

    return StatesMatchAtTolerance(actual_retry, expected_retry, 1.0e-12);
}

} // namespace

bool RunRollbackCase() noexcept
{
    const StateArrays initial = MakeRollbackState();
    blitzar_sim::Sim simulation(ParticleCount);
    blitzar_sim::Sim expected(ParticleCount);

    if (!PrepareRollbackPair(simulation, expected, initial)) {
        return false;
    }

    StateArrays before_failure{};

    if (!TriggerRollbackFailure(simulation, before_failure)) {
        return false;
    }

    StateArrays restored{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(restored)) != BLITZAR_STATUS_OK ||
        !StatesMatchAtTolerance(restored, before_failure, 1.0e-12)) {
        return false;
    }

    return ValidateRollbackRetry(simulation, expected);
}

} // namespace blitzar_mpi_tests
