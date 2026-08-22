#ifndef BLITZAR_PARALLEL_DOMAIN_DECOMPOSITION_HPP
#define BLITZAR_PARALLEL_DOMAIN_DECOMPOSITION_HPP

#include "core/Types.hpp"
#include "parallel/MpiContext.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_parallel {

struct DomainBounds final {
    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};

    [[nodiscard]] bool IsValid() const noexcept;
};

class DomainDecomposition final {
public:
    DomainDecomposition() noexcept = default;

    [[nodiscard]] blitzar_status Initialize(
        blitzar_core::ParticleStateView global_state,
        const MpiContext& context) noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] DomainBounds GlobalBounds() const noexcept;
    [[nodiscard]] DomainBounds LocalBounds() const noexcept;
    [[nodiscard]] int Owner(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] int Owner(
        blitzar_core::Vector3 position,
        std::uint64_t particle_id) const noexcept;

    [[nodiscard]] blitzar_status LocalIndices(
        blitzar_core::ParticleStateView global_state,
        std::vector<std::size_t>& indices) const noexcept;

private:
    struct SplitKey final {
        std::uint64_t key{};
        std::uint64_t particle_id{};
    };

    [[nodiscard]] static DomainBounds BoundsOf(
        blitzar_core::ParticleStateView state) noexcept;
    [[nodiscard]] static bool Extend(
        DomainBounds& bounds, blitzar_core::Vector3 position) noexcept;

    int rank_{0};
    int size_{1};
    bool initialized_{false};
    DomainBounds global_bounds_{};
    DomainBounds local_bounds_{};
    std::vector<SplitKey> split_keys_;
};

}  // namespace blitzar_parallel

#endif
