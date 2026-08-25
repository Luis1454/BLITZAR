#include "parallel/MpiNative.hpp"
#include "parallel/MpiNativeState.hpp"

#include <climits>

#if defined(BLITZAR_HAS_MPI)

namespace blitzar_parallel {

blitzar_status MpiNative::ReduceMaxInt(int local_value, int& global_value) const noexcept
{
    return MPI_Allreduce(&local_value, &global_value, 1, MPI_INT, MPI_MAX, impl_->communicator) ==
                   MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum, std::span<blitzar_core::Scalar> maximum) const noexcept
{
    if (minimum.size() != 3 || maximum.size() != 3) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Allreduce(MPI_IN_PLACE, minimum.data(), 3, MPI_DOUBLE, MPI_MIN,
               impl_->communicator) == MPI_SUCCESS &&
                   MPI_Allreduce(MPI_IN_PLACE, maximum.data(), 3, MPI_DOUBLE, MPI_MAX,
                       impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::BroadcastScalars(
    std::span<blitzar_core::Scalar> values, int root) const noexcept
{
    if (values.size() > static_cast<std::size_t>(INT_MAX)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Bcast(values.data(), static_cast<int>(values.size()), MPI_DOUBLE, root,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::BroadcastIds(std::span<std::uint64_t> values, int root) const noexcept
{
    if (values.size() > static_cast<std::size_t>(INT_MAX)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Bcast(values.data(), static_cast<int>(values.size()), MPI_UINT64_T, root,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_parallel

#else

namespace blitzar_parallel {

blitzar_status MpiNative::ReduceMaxInt(int local_value, int& global_value) const noexcept
{
    (void)local_value;
    (void)global_value;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum, std::span<blitzar_core::Scalar> maximum) const noexcept
{
    (void)minimum;
    (void)maximum;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::BroadcastScalars(
    std::span<blitzar_core::Scalar> values, int root) const noexcept
{
    (void)values;
    (void)root;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::BroadcastIds(std::span<std::uint64_t> values, int root) const noexcept
{
    (void)values;
    (void)root;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_parallel

#endif
