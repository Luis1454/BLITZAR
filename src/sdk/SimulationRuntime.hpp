#ifndef BLITZAR_SDK_SIMULATION_RUNTIME_HPP
#define BLITZAR_SDK_SIMULATION_RUNTIME_HPP

#include "gpu/HipContext.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiExchange.hpp"

#include <cstddef>

namespace blitzar_sdk {

class SimulationRuntime final {
public:
    explicit SimulationRuntime(std::size_t particle_count);

    SimulationRuntime(const SimulationRuntime&) = delete;
    SimulationRuntime& operator=(const SimulationRuntime&) = delete;
    SimulationRuntime(SimulationRuntime&&) = delete;
    SimulationRuntime& operator=(SimulationRuntime&&) = delete;

    [[nodiscard]] blitzar_parallel::MpiContext& Mpi() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiContext& Mpi() const noexcept;
    [[nodiscard]] blitzar_parallel::DomainDecomposition& Domain() noexcept;
    [[nodiscard]] const blitzar_parallel::DomainDecomposition& Domain() const noexcept;
    [[nodiscard]] blitzar_parallel::MpiExchange& Exchange() noexcept;
    [[nodiscard]] const blitzar_parallel::MpiExchange& Exchange() const noexcept;
    [[nodiscard]] blitzar_gpu::HipContext& Hip() noexcept;
    [[nodiscard]] const blitzar_gpu::HipContext& Hip() const noexcept;

private:
    blitzar_parallel::MpiContext mpi_context_;
    blitzar_parallel::DomainDecomposition domain_;
    blitzar_parallel::MpiExchange mpi_exchange_;
    blitzar_gpu::HipContext hip_context_;
};

} // namespace blitzar_sdk

#endif
