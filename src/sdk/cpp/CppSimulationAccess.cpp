#include "sdk/cpp/CppSimulationAccess.hpp"

#include "sdk/c/CApiState.hpp"

namespace blitzar {

blitzar_status CppSimulationAccess::GetLocalState(Simulation& simulation,
    blitzar_core::ParticleOutputView output, std::span<std::uint64_t> ids,
    std::size_t& count) noexcept
{
    count = 0;

    if (simulation.impl_ == nullptr || simulation.impl_->handle == nullptr) {
        return static_cast<blitzar_status>(simulation.Update(BLITZAR_STATUS_INVALID_ARGUMENT));
    }

    blitzar_sdk_api::SimulationCallGuard guard(*simulation.impl_->handle);

    if (!guard.Acquired()) {
        return static_cast<blitzar_status>(simulation.Update(BLITZAR_STATUS_INTERNAL_ERROR));
    }

    return static_cast<blitzar_status>(simulation.Update(
        simulation.impl_->handle->implementation.GetLocalState(output, ids, count)));
}

} // namespace blitzar
