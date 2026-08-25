#include "parallel/Partition.hpp"

#include "trees/Morton.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>

namespace blitzar_parallel {

blitzar_status Partition::AllocateBuffers(
    std::size_t source_count, const MpiContext& context) noexcept
{
    try {
        const std::size_t split_count = size_ > 1 ? static_cast<std::size_t>(size_ - 1) : 0;

        split_keys_.resize(split_count);
        split_values_.resize(split_count);
        split_ids_.resize(split_count);
        keys_.clear();
        order_.clear();

        if (context.Rank() == 0) {
            keys_.resize(source_count);
            order_.resize(source_count);
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

void Partition::BuildRootKeys(
    blitzar_core::ParticleStateView state, DomainBounds global_bounds) noexcept
{
    for (std::size_t index = 0; index < state.SourceCount(); ++index) {
        keys_[index] = blitzar_trees::MortonKey(
            {state.x[index], state.y[index], state.z[index]}, global_bounds.minimum,
            global_bounds.maximum);
        order_[index] = index;
    }

    std::sort(order_.begin(), order_.end(), [this](const auto left, const auto right) {
        return keys_[left] < keys_[right] || (keys_[left] == keys_[right] && left < right);
    });
}

void Partition::BuildRootSplits() noexcept
{
    for (int destination = 1; destination < size_; ++destination) {
        const std::size_t split_index = static_cast<std::size_t>(destination - 1);

        if (order_.empty()) {
            split_keys_[split_index] = {};
            continue;
        }

        const std::size_t boundary = std::min(order_.size() - 1,
            static_cast<std::size_t>(destination) * order_.size() /
                static_cast<std::size_t>(size_));
        split_keys_[split_index] = {
            keys_[order_[boundary]], static_cast<std::uint64_t>(order_[boundary])};
    }
}

blitzar_status Partition::BroadcastSplits(const MpiContext& context) noexcept
{
    for (std::size_t index = 0; index < split_keys_.size(); ++index) {
        split_values_[index] = split_keys_[index].key;
        split_ids_[index] = split_keys_[index].particle_id;
    }

    blitzar_status status = context.Broadcast(split_values_, 0);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = context.Broadcast(split_ids_, 0);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (std::size_t index = 0; index < split_keys_.size(); ++index) {
        split_keys_[index] = {split_values_[index], split_ids_[index]};
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Partition::Initialize(blitzar_core::ParticleStateView state,
    DomainBounds global_bounds, const MpiContext& context) noexcept
{
    size_ = context.Size();
    split_keys_.clear();

    const blitzar_status allocation_status = AllocateBuffers(state.SourceCount(), context);
    blitzar_status global_allocation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = context.SynchronizeStatus(allocation_status,
        "Partition", "initialize-allocation", global_allocation_status);

    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_allocation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                            : global_allocation_status;
    }

    if (context.Rank() == 0) {
        BuildRootKeys(state, global_bounds);
        BuildRootSplits();
    }

    return BroadcastSplits(context);
}

int Partition::Owner(blitzar_core::Vector3 position, std::uint64_t particle_id,
    DomainBounds global_bounds) const noexcept
{
    if (size_ <= 1) {
        return 0;
    }

    const SplitKey candidate{blitzar_trees::MortonKey(
                                 position, global_bounds.minimum, global_bounds.maximum),
        particle_id};
    const auto boundary = std::upper_bound(split_keys_.begin(), split_keys_.end(), candidate,
        [](const SplitKey& left, const SplitKey& right) {
            return left.key < right.key ||
                   (left.key == right.key && left.particle_id < right.particle_id);
        });

    return static_cast<int>(boundary - split_keys_.begin());
}

} // namespace blitzar_parallel
