#include "mpi/native/MpiNativeStatus.hpp"
#include "simulation/Sim.hpp"
#include "simulation/initialization/SimInputStage.hpp"
#include "simulation/step/SimPacketStoreRequest.hpp"

#include <new>
#include <span>
#include <stdexcept>
#include <utility>

namespace blitzar_sim {

bool Sim::ValidateParticleInput(blitzar_core::ParticleStateView input) const noexcept
{
    const bool root = runtime_.Mpi().Rank() == 0;

    return root ? input.count == particle_count_ && blitzar_core::IsValid(input)
                : (input.count == 0 && blitzar_core::IsValid(input)) ||
                      (input.count == particle_count_ && blitzar_core::IsValid(input));
}

blitzar_status Sim::DistributeParticles(SimInputStage& stage,
    blitzar_parallel::MpiDomainDecomposition& domain,
    blitzar_parallel::PacketBuffer& distributed) noexcept
{
    const bool root = runtime_.Mpi().Rank() == 0;
    const std::size_t local_capacity = LocalCapacity(particle_count_, runtime_.Mpi().Size());
    const std::size_t distribution_capacity = root ? particle_count_ : local_capacity;
    blitzar_status status = BLITZAR_STATUS_OK;

    try {
        if (!distributed.EnsureCapacity(local_capacity)) {
            status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }

        blitzar_parallel::MpiContext distribution_context;

        status = status == BLITZAR_STATUS_OK ? distribution_context.Status() : status;

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        blitzar_parallel::MpiExchange distribution_exchange(
            distribution_context, domain, distribution_capacity);

        std::vector<std::uint64_t> root_ids;

        if (root) {
            root_ids.resize(particle_count_);

            for (std::size_t index = 0; index < particle_count_; ++index) {
                root_ids[index] = static_cast<std::uint64_t>(index);
            }
        }

        const std::span<const std::uint64_t> ids =
            root ? std::span<const std::uint64_t>(root_ids) : std::span<const std::uint64_t>{};

        return distribution_exchange.Migrate(stage.State(), ids, distributed);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status Sim::SetParticles(blitzar_core::ParticleStateView input) noexcept
{
    if (!runtime_.Mpi().IsUsable()) {
        return Remember(runtime_.Mpi().Status());
    }

    blitzar_status status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(),
        ValidateParticleInput(input) ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "set-particles-input");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    SimInputStage stage;
    const bool root = runtime_.Mpi().Rank() == 0;

    status = root ? StageParticleInput(input, stage) : BLITZAR_STATUS_OK;
    status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(), status, "set-particles-stage");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    blitzar_parallel::MpiDomainDecomposition candidate_domain;

    status = candidate_domain.Initialize(stage.State(), runtime_.Mpi());
    status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(), status, "set-particles-domain");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    blitzar_parallel::PacketBuffer distributed_packets;

    status = DistributeParticles(stage, candidate_domain, distributed_packets);
    status =
        blitzar_parallel::SynchronizeStatus(runtime_.Mpi(), status, "set-particles-distribution");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }
    if (distributed_packets.Size() > particle_state_.Arena().Count() ||
        distributed_packets.Size() > particle_ids_.size()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    SimPacketStoreRequest store_request{distributed_packets, particle_state_.Arena(),
        particle_state_.Particles(), particle_state_.Accelerations(), particle_state_.Checkpoint(),
        std::span<std::uint64_t>(particle_ids_), particle_count_, local_particle_count_};

    status = StoreLocalPackets(store_request);
    status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(), status, "set-particles-store");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    runtime_.Domain() = std::move(candidate_domain);

    (void)particle_source_.SetCount(0);

    exchange_buffer_.Clear();

    particles_ready_ = true;

    return Remember(BLITZAR_STATUS_OK);
}

} // namespace blitzar_sim
