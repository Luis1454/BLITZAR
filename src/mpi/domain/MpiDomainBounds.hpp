#ifndef BLITZAR_MPI_DOMAIN_MPI_DOMAIN_BOUNDS_HPP
#define BLITZAR_MPI_DOMAIN_MPI_DOMAIN_BOUNDS_HPP

#include "core/CoreTypes.hpp"

namespace blitzar_parallel {

struct MpiDomainBounds final {
    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] bool Contains(blitzar_core::Vector3 position) const noexcept;
    [[nodiscard]] bool Include(blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] static MpiDomainBounds From(blitzar_core::ParticleStateView state) noexcept;
};

} // namespace blitzar_parallel

#endif
