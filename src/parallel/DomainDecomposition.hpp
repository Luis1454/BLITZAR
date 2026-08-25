#ifndef BLITZAR_PARALLEL_DOMAIN_DECOMPOSITION_HPP
#define BLITZAR_PARALLEL_DOMAIN_DECOMPOSITION_HPP

#include "core/Types.hpp"
#include "parallel/Bounds.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/Partition.hpp"

#include <cstdint>
#include <vector>

namespace blitzar_parallel {

class DomainDecomposition final {
public:
    DomainDecomposition() noexcept = default;

    [[nodiscard]] blitzar_status Initialize(
        blitzar_core::ParticleStateView global_state, const MpiContext& context) noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] DomainBounds GlobalBounds() const noexcept;
    [[nodiscard]] DomainBounds LocalBounds() const noexcept;
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
    DomainBounds global_bounds_{};
    DomainBounds local_bounds_{};
    Partition partition_;
};

} // namespace blitzar_parallel

#endif
