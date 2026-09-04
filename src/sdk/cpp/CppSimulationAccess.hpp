#ifndef BLITZAR_SDK_CPP_CPP_SIMULATION_ACCESS_HPP
#define BLITZAR_SDK_CPP_CPP_SIMULATION_ACCESS_HPP

#include "core/CoreTypes.hpp"
#include "sdk/cpp/CppState.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar {

struct CppSimulationAccess final {
    [[nodiscard]] static blitzar_status GetLocalState(Simulation& simulation,
        blitzar_core::ParticleOutputView output, std::span<std::uint64_t> ids,
        std::size_t& count) noexcept;
};

} // namespace blitzar

#endif
