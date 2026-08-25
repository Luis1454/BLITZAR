#include "parallel/MpiStatus.hpp"
#include "sdk/Simulation.hpp"
#include "sdk/Transaction.hpp"

namespace blitzar_sdk {

blitzar_status Simulation::PrepareDistributedStep(
    std::size_t rollback_particle_count, StepTransaction& transaction) noexcept
{
    const bool state_valid = rollback_particle_count == local_particle_count_ &&
                             rollback_particle_count <= particle_ids_.size() &&
                             rollback_particle_count == particle_storage_.Accelerations().Count() &&
                             rollback_particle_count == particle_storage_.Checkpoint().Count() &&
                             rollback_particle_count <= particle_storage_.Arena().Count();

    const blitzar_status state_status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(),
        state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "step-state");

    if (state_status != BLITZAR_STATUS_OK) {
        return state_status;
    }

    const blitzar_status prepare_status = transaction.Prepare();

    return blitzar_parallel::SynchronizeStatus(runtime_.Mpi(), prepare_status, "step-prepare");
}

} // namespace blitzar_sdk
