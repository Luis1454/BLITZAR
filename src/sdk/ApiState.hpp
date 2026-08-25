#ifndef BLITZAR_SDK_API_STATE_HPP
#define BLITZAR_SDK_API_STATE_HPP

#include "sdk/Simulation.hpp"

#include <atomic>
#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <span>

struct blitzar_context {
    blitzar_status status;
};

struct blitzar_simulation final {
    explicit blitzar_simulation(std::size_t particle_count) : implementation(particle_count) {}

    blitzar_sdk::Simulation implementation;
    mutable std::atomic_flag call_active = ATOMIC_FLAG_INIT;
};

namespace blitzar_sdk_api {

class SimulationCallGuard final {
public:
    explicit SimulationCallGuard(const blitzar_simulation& simulation) noexcept
        : simulation_(simulation),
          acquired_(!simulation_.call_active.test_and_set(std::memory_order_acquire))
    {
    }

    ~SimulationCallGuard() noexcept
    {
        if (acquired_) {
            simulation_.call_active.clear(std::memory_order_release);
        }
    }

    SimulationCallGuard(const SimulationCallGuard&) = delete;
    SimulationCallGuard& operator=(const SimulationCallGuard&) = delete;

    [[nodiscard]] bool Acquired() const noexcept
    {
        return acquired_;
    }

private:
    const blitzar_simulation& simulation_;
    bool acquired_;
};

[[nodiscard]] bool TryConvertCount(std::int64_t value, std::size_t& converted) noexcept;
[[nodiscard]] bool IsValidSimulation(const blitzar_simulation* simulation) noexcept;

template <typename Scalar>
[[nodiscard]] std::span<Scalar> MakeSpan(Scalar* data, std::size_t count) noexcept
{
    return count == 0 ? std::span<Scalar>{} : std::span<Scalar>(data, count);
}

[[nodiscard]] bool HasV2Header(
    std::uint32_t struct_size, std::uint32_t abi_version, std::size_t minimum_size) noexcept;
[[nodiscard]] bool ConvertBarnesHutConfig(const blitzar_barnes_hut_config_v2& source,
    blitzar_barnes_hut::BarnesHutSettings& target) noexcept;
[[nodiscard]] bool ConvertParticleInput(
    const blitzar_particle_input_v2& source, blitzar_core::ParticleStateView& target) noexcept;
[[nodiscard]] bool ConvertParticleOutput(
    const blitzar_particle_output_v2& source, blitzar_core::ParticleOutputView& target) noexcept;

[[nodiscard]] blitzar_status ApplyBarnesHut(
    blitzar_simulation& simulation, blitzar_barnes_hut::BarnesHutSettings settings) noexcept;
[[nodiscard]] blitzar_status ApplyParticles(
    blitzar_simulation& simulation, blitzar_core::ParticleStateView input) noexcept;
[[nodiscard]] blitzar_status ApplyState(
    const blitzar_simulation& simulation, blitzar_core::ParticleOutputView output) noexcept;

} // namespace blitzar_sdk_api

#endif
