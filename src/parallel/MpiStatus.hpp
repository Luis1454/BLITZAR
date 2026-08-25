#ifndef BLITZAR_PARALLEL_MPI_STATUS_HPP
#define BLITZAR_PARALLEL_MPI_STATUS_HPP

#include "parallel/MpiContext.hpp"

#include <blitzar/blitzar.h>

namespace blitzar_parallel {

[[nodiscard]] blitzar_status SynchronizeStatus(
    const MpiContext& context, blitzar_status local_status, const char* phase) noexcept;

} // namespace blitzar_parallel

#endif
