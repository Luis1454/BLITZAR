#include "sdk/CppState.hpp"

#include <new>
#include <utility>

namespace blitzar {

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

} // namespace blitzar
