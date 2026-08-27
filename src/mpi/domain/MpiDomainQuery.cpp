#include "mpi/domain/MpiDomainDecomposition.hpp"

#include <limits>

namespace blitzar_parallel {

bool MpiDomainDecomposition::IsInitialized() const noexcept
{
    return initialized_;
}

int MpiDomainDecomposition::Rank() const noexcept
{
    return rank_;
}

int MpiDomainDecomposition::Size() const noexcept
{
    return size_;
}

MpiDomainBounds MpiDomainDecomposition::GlobalBounds() const noexcept
{
    return global_bounds_;
}

MpiDomainBounds MpiDomainDecomposition::LocalBounds() const noexcept
{
    return local_bounds_;
}

bool MpiDomainDecomposition::Contains(blitzar_core::Vector3 position) const noexcept
{
    return initialized_ && global_bounds_.Contains(position);
}

int MpiDomainDecomposition::Owner(blitzar_core::Vector3 position) const noexcept
{
    return Owner(position, std::numeric_limits<std::uint64_t>::max());
}

int MpiDomainDecomposition::Owner(
    blitzar_core::Vector3 position, std::uint64_t particle_id) const noexcept
{
    if (!Contains(position)) {
        return -1;
    }

    return partition_.Owner(position, particle_id, global_bounds_);
}

} // namespace blitzar_parallel
