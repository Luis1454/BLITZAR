#ifndef BLITZAR_PARALLEL_MPI_COLLECTIVES_HPP
#define BLITZAR_PARALLEL_MPI_COLLECTIVES_HPP

#include "core/Types.hpp"
#include "parallel/MpiSession.hpp"

#include <blitzar/blitzar.h>
#include <span>

namespace blitzar_parallel {

class MpiCollectives final {
public:
    explicit MpiCollectives(const MpiSession& session) noexcept;

    [[nodiscard]] blitzar_status SynchronizeStatus(blitzar_status local_status,
        const char* operation, const char* phase, blitzar_status& global_status) const noexcept;
    [[nodiscard]] blitzar_status ReduceBounds(std::span<blitzar_core::Scalar> minimum,
        std::span<blitzar_core::Scalar> maximum) const noexcept;
    [[nodiscard]] blitzar_status ReduceMax(int local_value, int& global_value) const noexcept;

private:
    const MpiSession& session_;
};

} // namespace blitzar_parallel

#endif
