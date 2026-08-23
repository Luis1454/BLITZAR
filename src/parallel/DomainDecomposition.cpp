#include "parallel/DomainDecomposition.hpp"

#include "trees/Morton.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <numeric>

namespace blitzar_parallel {

namespace {

constexpr blitzar_core::Scalar PositiveInfinity =
    std::numeric_limits<blitzar_core::Scalar>::infinity();

[[nodiscard]] bool IsFinitePosition(blitzar_core::Vector3 position) noexcept
{
    return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
}

} // namespace

bool DomainBounds::IsValid() const noexcept
{
    return IsFinitePosition(minimum) && IsFinitePosition(maximum) && minimum.x <= maximum.x &&
           minimum.y <= maximum.y && minimum.z <= maximum.z;
}

DomainBounds DomainDecomposition::BoundsOf(blitzar_core::ParticleStateView state) noexcept
{
    DomainBounds bounds{{PositiveInfinity, PositiveInfinity, PositiveInfinity},
        {-PositiveInfinity, -PositiveInfinity, -PositiveInfinity}};

    for (std::size_t index = 0; index < state.SourceCount(); ++index) {
        (void)Extend(bounds, {state.x[index], state.y[index], state.z[index]});
    }
    if (!bounds.IsValid()) {
        bounds = {};
    }

    return bounds;
}

bool DomainDecomposition::Extend(DomainBounds& bounds, blitzar_core::Vector3 position) noexcept
{
    if (!IsFinitePosition(position)) {
        return false;
    }

    bounds.minimum.x = std::min(bounds.minimum.x, position.x);
    bounds.minimum.y = std::min(bounds.minimum.y, position.y);
    bounds.minimum.z = std::min(bounds.minimum.z, position.z);
    bounds.maximum.x = std::max(bounds.maximum.x, position.x);
    bounds.maximum.y = std::max(bounds.maximum.y, position.y);
    bounds.maximum.z = std::max(bounds.maximum.z, position.z);

    return true;
}

blitzar_status DomainDecomposition::Initialize(
    blitzar_core::ParticleStateView global_state, const MpiContext& context) noexcept
{
    initialized_ = false;
    split_keys_.clear();
    rank_ = context.Rank();
    size_ = context.Size();
    if (!context.IsUsable()) {
        return context.Status();
    }
    const bool input_valid = size_ > 0 && blitzar_core::IsValid(global_state);
    blitzar_status global_input_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context.SynchronizeStatus(input_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
            "DomainDecomposition", "initialize-preflight", global_input_status);
    if (synchronization_status != BLITZAR_STATUS_OK || global_input_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_input_status;
    }

    DomainBounds bounds = BoundsOf(global_state);
    if (global_state.SourceCount() == 0) {
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
    if (global_state.SourceCount() == 0) {
        global_bounds_ = {};
    }

    std::vector<std::uint64_t> keys;
    std::vector<std::size_t> order;
    try {
        keys.resize(global_state.SourceCount());
        order.resize(global_state.SourceCount());
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (std::size_t index = 0; index < global_state.SourceCount(); ++index) {
        keys[index] = blitzar_trees::MortonKey(
            {global_state.x[index], global_state.y[index], global_state.z[index]},
            global_bounds_.minimum, global_bounds_.maximum);
        order[index] = index;
    }
    std::sort(order.begin(), order.end(), [&keys](const auto left, const auto right) {
        return keys[left] < keys[right] || (keys[left] == keys[right] && left < right);
    });

    try {
        split_keys_.resize(size_ > 1 ? static_cast<std::size_t>(size_ - 1) : 0);
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (int destination = 1; destination < size_; ++destination) {
        if (order.empty()) {
            split_keys_[static_cast<std::size_t>(destination - 1)] = {};

            continue;
        }

        const std::size_t boundary = std::min(order.size() - 1,
            static_cast<std::size_t>(destination) * order.size() / static_cast<std::size_t>(size_));

        split_keys_[static_cast<std::size_t>(destination - 1)] = {
            keys[order[boundary]], static_cast<std::uint64_t>(order[boundary])};
    }

    initialized_ = true;
    local_bounds_ = {};
    for (std::size_t index = 0; index < global_state.SourceCount(); ++index) {
        if (Owner({global_state.x[index], global_state.y[index], global_state.z[index]},

                static_cast<std::uint64_t>(index)) == rank_) {
            (void)Extend(local_bounds_,
                {global_state.x[index], global_state.y[index], global_state.z[index]});
        }
    }
    if (!local_bounds_.IsValid()) {
        local_bounds_ = global_bounds_;
    }
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
    return initialized_ && global_bounds_.IsValid() && IsFinitePosition(position) &&
           position.x >= global_bounds_.minimum.x && position.x <= global_bounds_.maximum.x &&
           position.y >= global_bounds_.minimum.y && position.y <= global_bounds_.maximum.y &&
           position.z >= global_bounds_.minimum.z && position.z <= global_bounds_.maximum.z;
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
    if (size_ <= 1) {
        return 0;
    }
    const std::uint64_t key =
        blitzar_trees::MortonKey(position, global_bounds_.minimum, global_bounds_.maximum);
    const SplitKey candidate{key, particle_id};
    const auto boundary = std::upper_bound(split_keys_.begin(), split_keys_.end(), candidate,
        [](const SplitKey& left, const SplitKey& right) {
            return left.key < right.key ||
                   (left.key == right.key && left.particle_id < right.particle_id);
        });
    return static_cast<int>(boundary - split_keys_.begin());
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
