#ifndef BLITZAR_SIMULATION_COMPOSITION_SIMULATION_RESOURCES_HPP
#define BLITZAR_SIMULATION_COMPOSITION_SIMULATION_RESOURCES_HPP

#include "accelerators/gpu/hip/runtime/Context.hpp"
#include "parallel/mpi/domain/DomainDecomposition.hpp"
#include "parallel/mpi/exchange/MpiExchange.hpp"

#include <cstddef>

namespace blitzar_sim {

class SimulationResources final {
public:
    explicit SimulationResources(std::size_t particle_count);

    SimulationResources(const SimulationResources&) = delete;
    SimulationResources& operator=(const SimulationResources&) = delete;
    SimulationResources(SimulationResources&&) = delete;
    SimulationResources& operator=(SimulationResources&&) = delete;

    [[nodiscard]] blitzar_parallel::MpiContext& Mpi() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiContext& Mpi() const noexcept;
    [[nodiscard]] blitzar_parallel::DomainDecomposition& Domain() noexcept;
    [[nodiscard]] const blitzar_parallel::DomainDecomposition& Domain() const noexcept;
    [[nodiscard]] blitzar_parallel::MpiExchange& Exchange() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiExchange& Exchange() const noexcept;
    [[nodiscard]] blitzar_hip::Context& Accelerator() noexcept;
    [[nodiscard]] const blitzar_hip::Context& Accelerator() const noexcept;

private:
    blitzar_parallel::MpiContext mpi_context_;
    blitzar_parallel::DomainDecomposition domain_;
    blitzar_parallel::MpiExchange mpi_exchange_;
    blitzar_hip::Context accelerator_context_;
};

} // namespace blitzar_sim

#endif
