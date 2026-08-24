#include "sdk/Simulation.hpp"

#include "sdk/State.hpp"

#include "particles/ParticleArena.hpp"

#include <algorithm>
#include <cmath>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>

namespace blitzar_sdk {

blitzar_status Simulation::SetParticles(blitzar_core::ParticleStateView input) noexcept
{
    if (!mpi_context_.IsUsable()) {
        return Remember(mpi_context_.Status());
    }

    const bool root = mpi_context_.Rank() == 0;
    const bool input_sizes_valid = root
                                       ? input.count == particle_count_ && blitzar_core::IsValid(input)
                                       : (input.count == 0 && blitzar_core::IsValid(input)) ||
                                             (input.count == particle_count_ &&
                                                 blitzar_core::IsValid(input));

    blitzar_status input_status = SynchronizeSimulationStatus(mpi_context_,
        input_sizes_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "set-particles-input");

    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    ParticleInputStage stage;

    input_status = root ? StageParticleInput(input, stage) : BLITZAR_STATUS_OK;
    input_status = SynchronizeSimulationStatus(mpi_context_, input_status, "set-particles-stage");

    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    blitzar_parallel::DomainDecomposition candidate_domain;
    blitzar_status domain_status = candidate_domain.Initialize(stage.State(), mpi_context_);

    domain_status =
        SynchronizeSimulationStatus(mpi_context_, domain_status, "set-particles-domain");

    if (domain_status != BLITZAR_STATUS_OK) {
        return Remember(domain_status);
    }

    const std::size_t local_capacity = LocalCapacity(particle_count_, mpi_context_.Size());
    const std::size_t distribution_capacity = root ? particle_count_ : local_capacity;
    blitzar_parallel::PacketBuffer distributed_packets;
    blitzar_status distribution_status = BLITZAR_STATUS_OK;

    try {
        if (!distributed_packets.EnsureCapacity(local_capacity)) {
            distribution_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }

        blitzar_parallel::MpiContext distribution_context;

        distribution_status = distribution_status == BLITZAR_STATUS_OK
                                  ? distribution_context.Status()
                                  : distribution_status;

        distribution_status = SynchronizeSimulationStatus(
            mpi_context_, distribution_status, "set-particles-distribution-context");

        if (distribution_status == BLITZAR_STATUS_OK) {
            blitzar_parallel::MpiExchange distribution_exchange(
                distribution_context, candidate_domain, distribution_capacity);

            std::vector<std::uint64_t> root_ids;

            if (root) {
                root_ids.resize(particle_count_);

                for (std::size_t index = 0; index < particle_count_; ++index) {
                    root_ids[index] = static_cast<std::uint64_t>(index);
                }
            }

            const std::span<const std::uint64_t> ids = root
                                                            ? std::span<const std::uint64_t>(root_ids)
                                                            : std::span<const std::uint64_t>{};

            distribution_status = distribution_exchange.Migrate(
                stage.State(), ids, distributed_packets);
        }
    }
    catch (const std::length_error&) {
        distribution_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        distribution_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    distribution_status = SynchronizeSimulationStatus(
        mpi_context_, distribution_status, "set-particles-distribution");

    if (distribution_status != BLITZAR_STATUS_OK) {
        return Remember(distribution_status);
    }

    if (distributed_packets.Size() > arena_.Count() ||
        distributed_packets.Size() > particle_ids_.size()) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    PacketStoreRequest store_request{distributed_packets, arena_, particles_, accelerations_,
        checkpoint_, std::span<std::uint64_t>(particle_ids_), particle_count_,
        local_particle_count_};

    blitzar_status store_status = StoreLocalPackets(store_request);

    store_status = SynchronizeSimulationStatus(mpi_context_, store_status, "set-particles-store");

    if (store_status != BLITZAR_STATUS_OK) {
        return Remember(store_status);
    }

    domain_ = std::move(candidate_domain);

    (void)source_.SetCount(0);

    exchange_buffer_.Clear();

    particles_ready_ = true;

    return Remember(BLITZAR_STATUS_OK);
}

blitzar_status Simulation::GetState(blitzar_core::ParticleOutputView output) const noexcept
{
    const bool output_valid = particles_ready_ && output.count >= particle_count_ &&
                              blitzar_core::IsValid(output);

    const blitzar_status output_status = SynchronizeSimulationStatus(mpi_context_,
        output_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "get-state-preflight");

    if (output_status != BLITZAR_STATUS_OK) {
        return Remember(output_status);
    }

    const bool local_state_valid = particles_.Count() == local_particle_count_ &&
                                   local_particle_count_ <= particle_ids_.size() &&
                                   blitzar_core::IsValid(particles_.State());

    const blitzar_status state_status = SynchronizeSimulationStatus(mpi_context_,
        local_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "get-state-state");

    if (state_status != BLITZAR_STATUS_OK) {
        return Remember(state_status);
    }

    if (!mpi_context_.IsDistributed()) {
        const blitzar_core::ParticleStateView state = particles_.State();

        std::copy_n(state.x.begin(), particle_count_, output.x.begin());
        std::copy_n(state.y.begin(), particle_count_, output.y.begin());
        std::copy_n(state.z.begin(), particle_count_, output.z.begin());
        std::copy_n(state.velocity_x.begin(), particle_count_, output.velocity_x.begin());
        std::copy_n(state.velocity_y.begin(), particle_count_, output.velocity_y.begin());
        std::copy_n(state.velocity_z.begin(), particle_count_, output.velocity_z.begin());
        std::copy_n(state.mass.begin(), particle_count_, output.mass.begin());

        return Remember(BLITZAR_STATUS_OK);
    }

    const blitzar_status gather_status = mpi_exchange_.Gather(particles_.State(),
        std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
        gathered_buffer_);

    if (gather_status != BLITZAR_STATUS_OK) {
        return Remember(gather_status);
    }

    if (gathered_buffer_.Size() != particle_count_ || seen_.size() != particle_count_) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }

    std::fill(seen_.begin(), seen_.end(), 0);

    for (const blitzar_parallel::ParticlePacket& packet : gathered_buffer_.View()) {
        if (packet.id >= particle_count_ || seen_[packet.id] != 0 || !std::isfinite(packet.x) ||
            !std::isfinite(packet.y) || !std::isfinite(packet.z) ||
            !std::isfinite(packet.velocity_x) || !std::isfinite(packet.velocity_y) ||
            !std::isfinite(packet.velocity_z) || !std::isfinite(packet.mass) || packet.mass < 0.0) {
            return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
        }

        seen_[packet.id] = 1;
        output.x[packet.id] = packet.x;
        output.y[packet.id] = packet.y;
        output.z[packet.id] = packet.z;
        output.velocity_x[packet.id] = packet.velocity_x;
        output.velocity_y[packet.id] = packet.velocity_y;
        output.velocity_z[packet.id] = packet.velocity_z;
        output.mass[packet.id] = packet.mass;
    }

    if (std::find(seen_.begin(), seen_.end(), 0) != seen_.end()) {
        return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
    }

    return Remember(BLITZAR_STATUS_OK);
}

} // namespace blitzar_sdk
