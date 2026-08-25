#include "parallel/DomainDecomposition.hpp"

#include <array>

namespace blitzar_parallel {

blitzar_status DomainDecomposition::ValidateInput(
    blitzar_core::ParticleStateView state, const MpiContext& context) const noexcept
{
    const bool root = context.Rank() == 0;
    const bool valid =
        context.Size() > 0 && (root ? blitzar_core::IsValid(state)
                                    : state.SourceCount() == 0 || blitzar_core::IsValid(state));

    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context.SynchronizeStatus(valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
            "DomainDecomposition", "initialize-preflight", global_status);

    if (synchronization_status != BLITZAR_STATUS_OK) {
        return synchronization_status;
    }

    return global_status;
}

blitzar_status DomainDecomposition::InitializeBounds(
    blitzar_core::ParticleStateView state, const MpiContext& context) noexcept
{
    DomainBounds bounds = DomainBounds::From(state);

    if (state.SourceCount() == 0) {
        bounds = {};
    }

    std::array<blitzar_core::Scalar, 3> minimum{
        bounds.minimum.x, bounds.minimum.y, bounds.minimum.z};

    std::array<blitzar_core::Scalar, 3> maximum{
        bounds.maximum.x, bounds.maximum.y, bounds.maximum.z};

    const blitzar_status reduction_status = context.ReduceBounds(minimum, maximum);

    if (reduction_status != BLITZAR_STATUS_OK) {
        return reduction_status;
    }

    global_bounds_ = {{minimum[0], minimum[1], minimum[2]}, {maximum[0], maximum[1], maximum[2]}};

    int global_has_particles = 0;
    const blitzar_status presence_status = context.ReduceMax(
        context.Rank() == 0 && state.SourceCount() != 0 ? 1 : 0, global_has_particles);

    if (presence_status != BLITZAR_STATUS_OK) {
        return presence_status;
    }
    if (global_has_particles == 0) {
        global_bounds_ = {};
    }

    return BLITZAR_STATUS_OK;
}

void DomainDecomposition::UpdateLocalBounds(blitzar_core::ParticleStateView state) noexcept
{
    local_bounds_ = {};

    for (std::size_t index = 0; index < state.SourceCount(); ++index) {
        const blitzar_core::Vector3 position{state.x[index], state.y[index], state.z[index]};

        if (Owner(position, static_cast<std::uint64_t>(index)) == rank_) {
            (void)local_bounds_.Include(position);
        }
    }

    if (!local_bounds_.IsValid()) {
        local_bounds_ = global_bounds_;
    }
}

} // namespace blitzar_parallel
