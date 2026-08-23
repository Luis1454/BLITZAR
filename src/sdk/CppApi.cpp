#include <blitzar/blitzar.hpp>
#include <new>
#include <utility>

namespace blitzar {

struct Context::Impl final {
    struct Deleter final {
        void operator()(blitzar_context* context) const noexcept
        {
            blitzar_context_destroy(context);
        }
    };

    std::unique_ptr<blitzar_context, Deleter> handle;
};

struct Simulation::Impl final {
    struct Deleter final {
        void operator()(blitzar_simulation* simulation) const noexcept
        {
            blitzar_simulation_destroy(simulation);
        }
    };

    std::unique_ptr<blitzar_simulation, Deleter> handle;
};

Context::Context() noexcept : impl_(nullptr), status_(Status::InvalidArgument)
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = Status::AllocationFailure;
        return;
    }

    blitzar_context* context = nullptr;

    const blitzar_status status = blitzar_context_create(&context);

    impl_->handle.reset(context);
    status_ = FromCStatus(status);
}

Context::~Context() noexcept = default;

Context::Context(Context&& other) noexcept : impl_(std::move(other.impl_)), status_(other.status_)
{
    other.status_ = Status::InvalidArgument;
}

Context& Context::operator=(Context&& other) noexcept
{
    if (this != &other) {
        impl_ = std::move(other.impl_);
        status_ = other.status_;
        other.status_ = Status::InvalidArgument;
    }
    return *this;
}

bool Context::valid() const noexcept
{
    return impl_ != nullptr && impl_->handle != nullptr && status_ == Status::Ok;
}

Status Context::status() const noexcept
{
    return status_;
}

Simulation::Simulation(Context& context, std::int64_t particle_count) noexcept
    : impl_(nullptr), status_(Status::InvalidArgument), particle_count_(particle_count)
{
    if (!context.valid()) {
        return;
    }
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = Status::AllocationFailure;
        return;
    }

    blitzar_simulation* simulation = nullptr;

    const blitzar_status status =
        blitzar_simulation_create(context.impl_->handle.get(), particle_count, &simulation);

    impl_->handle.reset(simulation);
    status_ = FromCStatus(status);
}

Simulation::~Simulation() noexcept = default;

Simulation::Simulation(Simulation&& other) noexcept
    : impl_(std::move(other.impl_)), status_(other.status_.load(std::memory_order_relaxed)),
      particle_count_(other.particle_count_)
{
    other.status_.store(Status::InvalidArgument, std::memory_order_relaxed);
    other.particle_count_ = 0;
}

Simulation& Simulation::operator=(Simulation&& other) noexcept
{
    if (this != &other) {
        impl_ = std::move(other.impl_);
        status_.store(other.status_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        particle_count_ = other.particle_count_;
        other.status_.store(Status::InvalidArgument, std::memory_order_relaxed);
        other.particle_count_ = 0;
    }
    return *this;
}

bool Simulation::valid() const noexcept
{
    return impl_ != nullptr && impl_->handle != nullptr;
}

Status Simulation::status() const noexcept
{
    return status_.load(std::memory_order_relaxed);
}

std::int64_t Simulation::particle_count() const noexcept
{
    return particle_count_;
}

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

Status Simulation::set_barnes_hut(double opening_angle, std::int64_t max_particles,
    std::int64_t max_cells, std::int64_t leaf_capacity, std::int64_t max_depth) noexcept
{
    return Update(
        blitzar_simulation_set_barnes_hut(impl_ == nullptr ? nullptr : impl_->handle.get(),
            opening_angle, max_particles, max_cells, leaf_capacity, max_depth));
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

Status Simulation::set_particles(std::span<const double> position_x,
    std::span<const double> position_y, std::span<const double> position_z,
    std::span<const double> velocity_x, std::span<const double> velocity_y,
    std::span<const double> velocity_z, std::span<const double> mass) noexcept
{
    if (position_x.size() != position_y.size() || position_x.size() != position_z.size() ||
        position_x.size() != velocity_x.size() || position_x.size() != velocity_y.size() ||
        position_x.size() != velocity_z.size() || position_x.size() != mass.size() ||
        !FitsCount(position_x.size())) {
        return Update(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    return Update(blitzar_simulation_set_particles(impl_ == nullptr ? nullptr : impl_->handle.get(),
        static_cast<std::int64_t>(position_x.size()), position_x.data(), position_y.data(),
        position_z.data(), velocity_x.data(), velocity_y.data(), velocity_z.data(), mass.data()));
}

Status Simulation::get_state(std::span<double> position_x, std::span<double> position_y,
    std::span<double> position_z, std::span<double> velocity_x, std::span<double> velocity_y,
    std::span<double> velocity_z, std::span<double> mass) noexcept
{
    if (position_x.size() != position_y.size() || position_x.size() != position_z.size() ||
        position_x.size() != velocity_x.size() || position_x.size() != velocity_y.size() ||
        position_x.size() != velocity_z.size() || position_x.size() != mass.size() ||
        !FitsCount(position_x.size())) {
        return Update(BLITZAR_STATUS_INVALID_ARGUMENT);
    }
    return Update(blitzar_simulation_get_state(impl_ == nullptr ? nullptr : impl_->handle.get(),
        static_cast<std::int64_t>(position_x.size()), position_x.data(), position_y.data(),
        position_z.data(), velocity_x.data(), velocity_y.data(), velocity_z.data(), mass.data()));
}

Status Simulation::step() noexcept
{
    return Update(blitzar_simulation_step(impl_ == nullptr ? nullptr : impl_->handle.get()));
}

} // namespace blitzar
