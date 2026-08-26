#ifndef BLITZAR_PARALLEL_MPI_DOMAIN_DOMAIN_BOUNDS_HPP
#define BLITZAR_PARALLEL_MPI_DOMAIN_DOMAIN_BOUNDS_HPP

#include "core/contracts/Types.hpp"

namespace blitzar_parallel {

struct DomainBounds final {
    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] bool Contains(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] bool Include(blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] static DomainBounds From(blitzar_core::ParticleStateView state) noexcept;
};

} // namespace blitzar_parallel

#endif
