#include "parallel/mpi/collectives/MpiCollectives.hpp"

#include <climits>
#include <cstddef>

namespace blitzar_parallel {

namespace {

[[nodiscard]] bool ValidBroadcastLayout(int root, int peer_count, std::size_t value_count) noexcept
{
    return root >= 0 && root < peer_count && value_count <= static_cast<std::size_t>(INT_MAX);
}

} // namespace

blitzar_status MpiCollectives::Broadcast(
    std::span<blitzar_core::Scalar> values, int root) const noexcept
{
    const bool layout_valid = ValidBroadcastLayout(root, session_.Size(), values.size());

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        return layout_valid && root == 0 ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BroadcastScalars(values, root, layout_valid);
}

blitzar_status MpiCollectives::Broadcast(std::span<std::uint64_t> values, int root) const noexcept
{
    const bool layout_valid = ValidBroadcastLayout(root, session_.Size(), values.size());

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        return layout_valid && root == 0 ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BroadcastIds(values, root, layout_valid);
}

blitzar_status MpiCollectives::BroadcastScalars(
    std::span<blitzar_core::Scalar> values, int root, bool layout_valid) const noexcept
{
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        SynchronizeStatus(layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
            "MpiCollectives", "broadcast-scalar-layout", global_layout_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_layout_status;
    }

    return session_.Native().BroadcastScalars(values, root);
}

blitzar_status MpiCollectives::BroadcastIds(
    std::span<std::uint64_t> values, int root, bool layout_valid) const noexcept
{
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        SynchronizeStatus(layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
            "MpiCollectives", "broadcast-u64-layout", global_layout_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_layout_status;
    }

    return session_.Native().BroadcastIds(values, root);
}

} // namespace blitzar_parallel
