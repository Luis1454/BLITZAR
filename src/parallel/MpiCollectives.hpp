#ifndef BLITZAR_PARALLEL_MPI_COLLECTIVES_HPP
#define BLITZAR_PARALLEL_MPI_COLLECTIVES_HPP

#include "core/Types.hpp"
#include "parallel/MpiSession.hpp"

#include <blitzar/blitzar.h>
#include <cstdint>
#include <span>
#include <string_view>

namespace blitzar_parallel {

class MpiCollectives final {
public:
    explicit MpiCollectives(const MpiSession& session) noexcept;

    [[nodiscard]] blitzar_status SynchronizeStatus(blitzar_status local_status,
        std::string_view operation, std::string_view phase,
        blitzar_status& global_status) const noexcept;
    [[nodiscard]] blitzar_status ReduceBounds(std::span<blitzar_core::Scalar> minimum,
        std::span<blitzar_core::Scalar> maximum) const noexcept;
    [[nodiscard]] blitzar_status ReduceMax(int local_value, int& global_value) const noexcept;
    [[nodiscard]] blitzar_status Broadcast(
        std::span<blitzar_core::Scalar> values, int root) const noexcept;
    [[nodiscard]] blitzar_status Broadcast(
        std::span<std::uint64_t> values, int root) const noexcept;

private:
    [[nodiscard]] blitzar_status BroadcastScalars(
        std::span<blitzar_core::Scalar> values, int root, bool layout_valid) const noexcept;
    [[nodiscard]] blitzar_status BroadcastIds(
        std::span<std::uint64_t> values, int root, bool layout_valid) const noexcept;

    const MpiSession& session_;
};

} // namespace blitzar_parallel

#endif
