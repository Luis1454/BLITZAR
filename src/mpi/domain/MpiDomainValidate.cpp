#include "mpi/domain/MpiDomainDecomposition.hpp"

#include <new>

namespace blitzar_parallel {

blitzar_status MpiDomainDecomposition::ValidateState(
    blitzar_core::ParticleStateView state) const noexcept
{
    if (!initialized_ || !blitzar_core::IsValid(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < state.count; ++index) {
        if (!Contains({state.x[index], state.y[index], state.z[index]})) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiDomainDecomposition::LocalIndices(
    blitzar_core::ParticleStateView global_state, std::vector<std::size_t>& indices) const noexcept
{
    indices.clear();

    if (!initialized_ || !blitzar_core::IsValid(global_state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        indices.reserve(global_state.SourceCount() / static_cast<std::size_t>(size_) + 1);

        for (std::size_t index = 0; index < global_state.SourceCount(); ++index) {
            if (Owner({global_state.x[index], global_state.y[index], global_state.z[index]},

                    static_cast<std::uint64_t>(index)) == rank_) {
                indices.push_back(index);
            }
        }
    }
    catch (const std::bad_alloc&) {
        indices.clear();

        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
