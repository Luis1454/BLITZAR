#include "neighborhood/NeighborCase.hpp"

#include <cmath>

namespace {

std::uint64_t Next(std::uint64_t& state) noexcept
{
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;

    return state;
}

double Unit(std::uint64_t& state) noexcept
{
    constexpr double denominator = 9007199254740992.0;

    return static_cast<double>(Next(state) >> 11U) / denominator;
}

double Between(std::uint64_t& state, double minimum, double maximum) noexcept
{
    return minimum + (maximum - minimum) * Unit(state);
}

blitzar_core::Vector3 MakePosition(
    blitzar_neighborhood::ScenarioKind scenario, std::size_t index, std::uint64_t& state) noexcept
{
    if (scenario == blitzar_neighborhood::ScenarioKind::Dense) {
        return {Between(state, -2.0, 2.0), Between(state, -2.0, 2.0), Between(state, -2.0, 2.0)};
    }
    if (scenario == blitzar_neighborhood::ScenarioKind::Clustered) {
        const double center = index % 2U == 0U ? -4.0 : 4.0;

        return {Between(state, center - 0.8, center + 0.8), Between(state, -0.8, 0.8),
            Between(state, -0.8, 0.8)};
    }
    if (scenario == blitzar_neighborhood::ScenarioKind::Moving) {
        return {Between(state, -3.0, 3.0), Between(state, -3.0, 3.0), Between(state, -3.0, 3.0)};
    }

    return {Between(state, -8.0, 8.0), Between(state, -8.0, 8.0), Between(state, -8.0, 8.0)};
}

blitzar_core::Vector3 MakeVelocity(
    blitzar_neighborhood::ScenarioKind scenario, std::size_t index) noexcept
{
    if (scenario != blitzar_neighborhood::ScenarioKind::Moving) {
        return {};
    }

    const double sign = index % 2U == 0U ? 1.0 : -1.0;

    return {sign * 0.32, ((index / 2U) % 2U == 0U ? 1.0 : -1.0) * 0.21, 0.13};
}

blitzar_neighborhood::Bounds BoundsFor(blitzar_neighborhood::ScenarioKind scenario) noexcept
{
    if (scenario == blitzar_neighborhood::ScenarioKind::Dense ||
        scenario == blitzar_neighborhood::ScenarioKind::Clustered) {
        return {{-8.0, -8.0, -8.0}, {8.0, 8.0, 8.0}};
    }

    return {{-8.0, -8.0, -8.0}, {8.0, 8.0, 8.0}};
}

blitzar_neighborhood::NeighborWorkload MakeWorkload(
    std::uint64_t seed, blitzar_neighborhood::ScenarioKind scenario)
{
    blitzar_neighborhood::NeighborWorkload workload;

    workload.seed = seed;
    workload.scenario = scenario;
    workload.parameters = {BoundsFor(scenario), 0.75, 0.4, blitzar_neighborhood::kStepCount};

    workload.positions.reserve(blitzar_neighborhood::kParticleCount);
    workload.velocities.reserve(blitzar_neighborhood::kParticleCount);

    std::uint64_t state =
        seed ^ (0x9e3779b97f4a7c15ULL * (static_cast<std::uint64_t>(scenario) + 1U));

    for (std::size_t index = 0; index < blitzar_neighborhood::kParticleCount; ++index) {
        workload.positions.push_back(MakePosition(scenario, index, state));
        workload.velocities.push_back(MakeVelocity(scenario, index));
    }

    return workload;
}

} // namespace

namespace blitzar_neighborhood {

std::vector<NeighborWorkload> MakeWorkloads(std::uint64_t seed)
{
    return {MakeWorkload(seed, ScenarioKind::Dense), MakeWorkload(seed, ScenarioKind::Sparse),
        MakeWorkload(seed, ScenarioKind::Clustered), MakeWorkload(seed, ScenarioKind::Moving)};
}

NeighborFrame MakeFrame(const NeighborWorkload& workload, std::size_t step)
{
    NeighborFrame frame(workload.positions.size());

    for (std::size_t index = 0; index < workload.positions.size(); ++index) {
        const auto position = workload.positions[index];
        const auto velocity = workload.velocities[index];

        frame.x[index] = position.x + velocity.x * static_cast<double>(step);
        frame.y[index] = position.y + velocity.y * static_cast<double>(step);
        frame.z[index] = position.z + velocity.z * static_cast<double>(step);
        frame.velocity_x[index] = velocity.x;
        frame.velocity_y[index] = velocity.y;
        frame.velocity_z[index] = velocity.z;
    }

    return frame;
}

bool IsMoving(ScenarioKind scenario) noexcept
{
    return scenario == ScenarioKind::Moving;
}

} // namespace blitzar_neighborhood
