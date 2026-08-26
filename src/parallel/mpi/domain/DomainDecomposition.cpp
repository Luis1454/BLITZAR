#include "parallel/mpi/domain/DomainDecomposition.hpp"

namespace blitzar_parallel {

blitzar_status DomainDecomposition::Initialize(
    blitzar_core::ParticleStateView global_state, const MpiContext& context) noexcept
{
    initialized_ = false;
    rank_ = context.Rank();
    size_ = context.Size();
    global_bounds_ = {};
    local_bounds_ = {};

    if (!context.IsUsable()) {
        return context.Status();
    }

    const blitzar_status input_status = ValidateInput(global_state, context);

    if (input_status != BLITZAR_STATUS_OK) {
        return input_status;
    }

    const blitzar_status bounds_status = InitializeBounds(global_state, context);

    if (bounds_status != BLITZAR_STATUS_OK) {
        return bounds_status;
    }

    const blitzar_status partition_status =
        partition_.Initialize(global_state, global_bounds_, context);

    if (partition_status != BLITZAR_STATUS_OK) {
        return partition_status;
    }

    initialized_ = true;

    UpdateLocalBounds(global_state);

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
