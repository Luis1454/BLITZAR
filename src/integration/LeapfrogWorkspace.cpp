#include "integration/LeapfrogWorkspace.hpp"

namespace blitzar_integration {

LeapfrogWorkspace::LeapfrogWorkspace(std::size_t count)
    : count_(count),
      position_x_(count),
      position_y_(count),
      position_z_(count),
      velocity_x_(count),
      velocity_y_(count),
      velocity_z_(count)
{
}

std::size_t LeapfrogWorkspace::Count() const noexcept
{
    return count_;
}

blitzar_status LeapfrogWorkspace::Capture(
    blitzar_core::MutableParticleView state) noexcept
{
    if (state.count != count_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    for (std::size_t index = 0; index < count_; ++index) {
        position_x_.Data()[index] = state.x[index];
        position_y_.Data()[index] = state.y[index];
        position_z_.Data()[index] = state.z[index];
        velocity_x_.Data()[index] = state.velocity_x[index];
        velocity_y_.Data()[index] = state.velocity_y[index];
        velocity_z_.Data()[index] = state.velocity_z[index];
    }
    return BLITZAR_STATUS_OK;
}

blitzar_status LeapfrogWorkspace::Restore(
    blitzar_core::MutableParticleView state) noexcept
{
    if (state.count != count_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    for (std::size_t index = 0; index < count_; ++index) {
        state.x[index] = position_x_.Data()[index];
        state.y[index] = position_y_.Data()[index];
        state.z[index] = position_z_.Data()[index];
        state.velocity_x[index] = velocity_x_.Data()[index];
        state.velocity_y[index] = velocity_y_.Data()[index];
        state.velocity_z[index] = velocity_z_.Data()[index];
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace blitzar_integration
