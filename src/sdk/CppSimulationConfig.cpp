#include "sdk/CppState.hpp"

namespace blitzar {

BackendKind Simulation::backend() const noexcept
{
    blitzar_backend_kind backend = BLITZAR_BACKEND_CPU;

    if (blitzar_simulation_backend(impl_ == nullptr ? nullptr : impl_->handle.get(), &backend) !=
        BLITZAR_STATUS_OK) {
        return BackendKind::Cpu;
    }

    return static_cast<BackendKind>(backend);
}

Status Simulation::Update(blitzar_status status) noexcept
{
    status_.store(FromCStatus(status), std::memory_order_relaxed);

    return status_.load(std::memory_order_relaxed);
}

Status Simulation::set_solver(SolverKind solver) noexcept
{
    return Update(blitzar_simulation_set_solver(impl_ == nullptr ? nullptr : impl_->handle.get(),
        static_cast<blitzar_solver_kind>(solver)));
}

Status Simulation::set_integrator(IntegratorKind integrator) noexcept
{
    return Update(
        blitzar_simulation_set_integrator(impl_ == nullptr ? nullptr : impl_->handle.get(),
            static_cast<blitzar_integrator_kind>(integrator)));
}

Status Simulation::set_gravity(double gravitational_constant, double softening) noexcept
{
    return Update(blitzar_simulation_set_gravity(
        impl_ == nullptr ? nullptr : impl_->handle.get(), gravitational_constant, softening));
}

Status Simulation::set_units(double length_scale, double mass_scale, double time_scale) noexcept
{
    return Update(blitzar_simulation_set_units(
        impl_ == nullptr ? nullptr : impl_->handle.get(), length_scale, mass_scale, time_scale));
}

Status Simulation::set_barnes_hut(BarnesHutSettings settings) noexcept
{
    const blitzar_barnes_hut_config_v2 config{sizeof(config), BLITZAR_ABI_VERSION_V2,
        settings.opening_angle, settings.max_particles, settings.max_cells, settings.leaf_capacity,
        settings.max_depth};

    return Update(blitzar_simulation_set_barnes_hut_v2(
        impl_ == nullptr ? nullptr : impl_->handle.get(), &config));
}

Status Simulation::set_timestep(double timestep) noexcept
{
    return Update(blitzar_simulation_set_timestep(
        impl_ == nullptr ? nullptr : impl_->handle.get(), timestep));
}

Status Simulation::set_seed(std::uint64_t seed) noexcept
{
    return Update(
        blitzar_simulation_set_seed(impl_ == nullptr ? nullptr : impl_->handle.get(), seed));
}

} // namespace blitzar
