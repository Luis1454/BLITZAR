#ifndef BLITZAR_SIMULATION_RUNTIME_SIM_RUNTIME_HPP
#define BLITZAR_SIMULATION_RUNTIME_SIM_RUNTIME_HPP

#include "gpu/runtime/GpuContext.hpp"
#include "mpi/domain/MpiDomainDecomposition.hpp"
#include "mpi/exchange/MpiExchange.hpp"

#include <cstddef>

namespace blitzar_sim {

class SimRuntime final {
public:
    explicit SimRuntime(std::size_t particle_count);

    SimRuntime(const SimRuntime&) = delete;
    SimRuntime& operator=(const SimRuntime&) = delete;
    SimRuntime(SimRuntime&&) = delete;
    SimRuntime& operator=(SimRuntime&&) = delete;

    [[nodiscard]] blitzar_parallel::MpiContext& Mpi() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiContext& Mpi() const noexcept;
    [[nodiscard]] blitzar_parallel::MpiDomainDecomposition& Domain() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiDomainDecomposition& Domain() const noexcept;
    [[nodiscard]] blitzar_parallel::MpiExchange& Exchange() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiExchange& Exchange() const noexcept;
    [[nodiscard]] blitzar_hip::GpuContext& Accelerator() noexcept;
    [[nodiscard]] const blitzar_hip::GpuContext& Accelerator() const noexcept;

private:
    blitzar_parallel::MpiContext mpi_context_;
    blitzar_parallel::MpiDomainDecomposition domain_;
    blitzar_parallel::MpiExchange mpi_exchange_;
    blitzar_hip::GpuContext accelerator_context_;
};

} // namespace blitzar_sim

#endif
