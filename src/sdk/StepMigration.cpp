#include "sdk/Simulation.hpp"
#include "sdk/State.hpp"

#include <span>

namespace blitzar_sdk {

blitzar_integration_kdk::DriftTransition Simulation::MigrateAfterDrift(
    std::size_t rollback_particle_count, blitzar_particles::ParticleBuffer& current_particles,
    blitzar_particles::AccelerationBuffer& current_accelerations,
    blitzar_integration::KdkCheckpoint& current_checkpoint) noexcept
{
    const bool migration_state_valid = current_particles.Count() == rollback_particle_count &&
                                       current_accelerations.Count() == rollback_particle_count &&
                                       current_checkpoint.Count() == rollback_particle_count &&
                                       local_particle_count_ == rollback_particle_count &&
                                       rollback_particle_count <= particle_ids_.size();

    blitzar_status migration_status = SynchronizeSimulationStatus(mpi_context_,
        migration_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR,
        "migrate-preflight");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    migration_status = mpi_exchange_.Migrate(current_particles.State(),
        std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
        migration_buffer_);

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    migration_status = EnsureLocalCapacity(migration_buffer_.Size());
    migration_status =
        SynchronizeSimulationStatus(mpi_context_, migration_status, "migrate-capacity");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    PacketStoreRequest migration_request{migration_buffer_, arena_, current_particles,
        current_accelerations, current_checkpoint, std::span<std::uint64_t>(particle_ids_),
        particle_count_, local_particle_count_};

    migration_status = StoreLocalPackets(migration_request);
    migration_status =
        SynchronizeSimulationStatus(mpi_context_, migration_status, "migrate-commit");

    if (migration_status != BLITZAR_STATUS_OK) {
        return {migration_status, false};
    }

    (void)source_.SetCount(0);

    return {BLITZAR_STATUS_OK, true};
}

} // namespace blitzar_sdk
