#include "parallel/MpiStatus.hpp"

namespace blitzar_parallel {

blitzar_status SynchronizeStatus(
    const MpiContext& context, blitzar_status local_status, std::string_view phase) noexcept
{
    if (!context.IsDistributed()) {
        return local_status;
    }

    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context.SynchronizeStatus(local_status, "Simulation", phase, global_status);

    return synchronization_status == BLITZAR_STATUS_OK ? global_status : synchronization_status;
}

} // namespace blitzar_parallel
