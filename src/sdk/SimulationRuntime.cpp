#include "sdk/SimulationRuntime.hpp"

namespace blitzar_sdk {

SimulationRuntime::SimulationRuntime(std::size_t particle_count)
    : mpi_context_(), domain_(),
      mpi_exchange_(mpi_context_, domain_, particle_count, particle_count), hip_context_()
{
}

blitzar_parallel::MpiContext& SimulationRuntime::Mpi() noexcept
{
    return mpi_context_;
}

const blitzar_parallel::MpiContext& SimulationRuntime::Mpi() const noexcept
{
    return mpi_context_;
}

blitzar_parallel::DomainDecomposition& SimulationRuntime::Domain() noexcept
{
    return domain_;
}

const blitzar_parallel::DomainDecomposition& SimulationRuntime::Domain() const noexcept
{
    return domain_;
}

blitzar_parallel::MpiExchange& SimulationRuntime::Exchange() noexcept
{
    return mpi_exchange_;
}

const blitzar_parallel::MpiExchange& SimulationRuntime::Exchange() const noexcept
{
    return mpi_exchange_;
}

blitzar_gpu::HipContext& SimulationRuntime::Hip() noexcept
{
    return hip_context_;
}

const blitzar_gpu::HipContext& SimulationRuntime::Hip() const noexcept
{
    return hip_context_;
}

} // namespace blitzar_sdk
