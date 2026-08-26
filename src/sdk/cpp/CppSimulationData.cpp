#include "sdk/cpp/CppState.hpp"

namespace blitzar {

Status Simulation::set_particles(ParticleInput input) noexcept
{
    if (!input.IsSized() || !FitsCount(input.position_x.size())) {
        return Update(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    const blitzar_particle_input_v2 view{sizeof(view), BLITZAR_ABI_VERSION_V2,
        static_cast<std::int64_t>(input.position_x.size()), input.position_x.data(),
        input.position_y.data(), input.position_z.data(), input.velocity_x.data(),
        input.velocity_y.data(), input.velocity_z.data(), input.mass.data()};

    return Update(blitzar_simulation_set_particles_v2(
        impl_ == nullptr ? nullptr : impl_->handle.get(), &view));
}

Status Simulation::get_state(ParticleOutput output) noexcept
{
    if (!output.IsSized() || !FitsCount(output.position_x.size())) {
        return Update(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    const blitzar_particle_output_v2 view{sizeof(view), BLITZAR_ABI_VERSION_V2,
        static_cast<std::int64_t>(output.position_x.size()), output.position_x.data(),
        output.position_y.data(), output.position_z.data(), output.velocity_x.data(),
        output.velocity_y.data(), output.velocity_z.data(), output.mass.data()};

    return Update(
        blitzar_simulation_get_state_v2(impl_ == nullptr ? nullptr : impl_->handle.get(), &view));
}

Status Simulation::step() noexcept
{
    return Update(blitzar_simulation_step(impl_ == nullptr ? nullptr : impl_->handle.get()));
}

} // namespace blitzar
