#include "parallel/DomainDecomposition.hpp"

#include <array>
#include <limits>

namespace blitzar_parallel {

blitzar_status DomainDecomposition::ValidateInput(
    blitzar_core::ParticleStateView state, const MpiContext& context) const noexcept
{
    const bool root = context.Rank() == 0;
    const bool valid = context.Size() > 0 &&
                       (root ? blitzar_core::IsValid(state)
                             : state.SourceCount() == 0 || blitzar_core::IsValid(state));
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = context.SynchronizeStatus(
        valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "DomainDecomposition",
        "initialize-preflight", global_status);

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

bool DomainDecomposition::IsInitialized() const noexcept
{
    return initialized_;
}

int DomainDecomposition::Rank() const noexcept
{
    return rank_;
}

int DomainDecomposition::Size() const noexcept
{
    return size_;
}

DomainBounds DomainDecomposition::GlobalBounds() const noexcept
{
    return global_bounds_;
}

DomainBounds DomainDecomposition::LocalBounds() const noexcept
{
    return local_bounds_;
}

bool DomainDecomposition::Contains(blitzar_core::Vector3 position) const noexcept
{
    return initialized_ && global_bounds_.Contains(position);
}

blitzar_status DomainDecomposition::ValidateState(
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

int DomainDecomposition::Owner(blitzar_core::Vector3 position) const noexcept
{
    return Owner(position, std::numeric_limits<std::uint64_t>::max());
}

int DomainDecomposition::Owner(
    blitzar_core::Vector3 position, std::uint64_t particle_id) const noexcept
{
    if (!Contains(position)) {
        return -1;
    }
    return partition_.Owner(position, particle_id, global_bounds_);
}

blitzar_status DomainDecomposition::LocalIndices(
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
