#include "simulation/runtime/SimRuntime.hpp"

namespace blitzar_sim {

SimRuntime::SimRuntime(std::size_t particle_count)
    : mpi_context_(), domain_(),
      mpi_exchange_(mpi_context_, domain_, particle_count, particle_count), accelerator_context_()
{
}

blitzar_parallel::MpiContext& SimRuntime::Mpi() noexcept
{
    return mpi_context_;
}

const blitzar_parallel::MpiContext& SimRuntime::Mpi() const noexcept
{
    return mpi_context_;
}

blitzar_parallel::MpiDomainDecomposition& SimRuntime::Domain() noexcept
{
    return domain_;
}

const blitzar_parallel::MpiDomainDecomposition& SimRuntime::Domain() const noexcept
{
    return domain_;
}

blitzar_parallel::MpiExchange& SimRuntime::Exchange() noexcept
{
    return mpi_exchange_;
}

const blitzar_parallel::MpiExchange& SimRuntime::Exchange() const noexcept
{
    return mpi_exchange_;
}

blitzar_hip::GpuContext& SimRuntime::Accelerator() noexcept
{
    return accelerator_context_;
}

const blitzar_hip::GpuContext& SimRuntime::Accelerator() const noexcept
{
    return accelerator_context_;
}

} // namespace blitzar_sim
