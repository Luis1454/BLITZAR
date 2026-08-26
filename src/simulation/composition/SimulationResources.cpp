#include "simulation/composition/SimulationResources.hpp"

namespace blitzar_sim {

SimulationResources::SimulationResources(std::size_t particle_count)
    : mpi_context_(), domain_(),
      mpi_exchange_(mpi_context_, domain_, particle_count, particle_count), accelerator_context_()
{
}

blitzar_parallel::MpiContext& SimulationResources::Mpi() noexcept
{
    return mpi_context_;
}

const blitzar_parallel::MpiContext& SimulationResources::Mpi() const noexcept
{
    return mpi_context_;
}

blitzar_parallel::DomainDecomposition& SimulationResources::Domain() noexcept
{
    return domain_;
}

const blitzar_parallel::DomainDecomposition& SimulationResources::Domain() const noexcept
{
    return domain_;
}

blitzar_parallel::MpiExchange& SimulationResources::Exchange() noexcept
{
    return mpi_exchange_;
}

const blitzar_parallel::MpiExchange& SimulationResources::Exchange() const noexcept
{
    return mpi_exchange_;
}

blitzar_hip::Context& SimulationResources::Accelerator() noexcept
{
    return accelerator_context_;
}

const blitzar_hip::Context& SimulationResources::Accelerator() const noexcept
{
    return accelerator_context_;
}

} // namespace blitzar_sim
