#include "mpi/native/MpiNativeStatus.hpp"
#include "simulation/Sim.hpp"
#include "simulation/transaction/SimTransaction.hpp"

namespace blitzar_sim {

blitzar_status Sim::PrepareDistributedStep(
    std::size_t rollback_particle_count, SimTransaction& transaction) noexcept
{
    const bool state_valid = rollback_particle_count == local_particle_count_ &&
                             rollback_particle_count <= particle_ids_.size() &&
                             rollback_particle_count == particle_state_.Accelerations().Count() &&
                             rollback_particle_count == particle_state_.Checkpoint().Count() &&
                             rollback_particle_count <= particle_state_.Arena().Count();

    const blitzar_status state_status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(),
        state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "step-state");

    if (state_status != BLITZAR_STATUS_OK) {
        return state_status;
    }

    const blitzar_status prepare_status = transaction.Prepare();

    return blitzar_parallel::SynchronizeStatus(runtime_.Mpi(), prepare_status, "step-prepare");
}

} // namespace blitzar_sim
