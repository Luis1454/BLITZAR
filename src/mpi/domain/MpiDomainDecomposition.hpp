#ifndef BLITZAR_MPI_DOMAIN_MPI_DOMAIN_DECOMPOSITION_HPP
#define BLITZAR_MPI_DOMAIN_MPI_DOMAIN_DECOMPOSITION_HPP

#include "core/CoreTypes.hpp"
#include "mpi/domain/MpiDomainBounds.hpp"
#include "mpi/domain/MpiDomainPartition.hpp"
#include "mpi/runtime/MpiContext.hpp"

#include <cstdint>
#include <vector>

namespace blitzar_parallel {

class MpiDomainDecomposition final {
public:
    MpiDomainDecomposition() noexcept = default;

    [[nodiscard]] blitzar_status Initialize(
        blitzar_core::ParticleStateView global_state, const MpiContext& context) noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] MpiDomainBounds GlobalBounds() const noexcept;
    [[nodiscard]] MpiDomainBounds LocalBounds() const noexcept;
    [[nodiscard]] bool Contains(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] blitzar_status ValidateState(
        blitzar_core::ParticleStateView state) const noexcept;
    [[nodiscard]] int Owner(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] int Owner(
        blitzar_core::Vector3 position, std::uint64_t particle_id) const noexcept;

    [[nodiscard]] blitzar_status LocalIndices(blitzar_core::ParticleStateView global_state,
        std::vector<std::size_t>& indices) const noexcept;

private:
    [[nodiscard]] blitzar_status ValidateInput(
        blitzar_core::ParticleStateView state, const MpiContext& context) const noexcept;
    [[nodiscard]] blitzar_status InitializeBounds(
        blitzar_core::ParticleStateView state, const MpiContext& context) noexcept;
    void UpdateLocalBounds(blitzar_core::ParticleStateView state) noexcept;

    int rank_{0};
    int size_{1};
    bool initialized_{false};
    MpiDomainBounds global_bounds_{};
    MpiDomainBounds local_bounds_{};
    MpiDomainPartition partition_;
};

} // namespace blitzar_parallel

#endif
