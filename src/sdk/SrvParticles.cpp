#include "sdk/Simulation.hpp"

#include "sdk/SrvState.hpp"

#include "particles/ParticleArena.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <utility>

namespace blitzar_sdk {

blitzar_status Simulation::SetParticles(blitzar_core::ParticleStateView input) noexcept
{
    if (!mpi_context_.IsUsable()) {
        return Remember(mpi_context_.Status());
    }

    const bool input_sizes_valid = input.count == particle_count_ && blitzar_core::IsValid(input);
    blitzar_status input_status = SynchronizeSimulationStatus(mpi_context_,
        input_sizes_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "set-particles-input");

    if (input_status != BLITZAR_STATUS_OK) {
        return Remember(input_status);
    }

    SrvParticleInputStage stage;

    input_status = SrvStageParticleInput(input, stage);
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

    local_indices_.clear();

    blitzar_status index_status = candidate_domain.LocalIndices(stage.State(), local_indices_);

    index_status = SynchronizeSimulationStatus(mpi_context_, index_status, "set-particles-indices");

    if (index_status != BLITZAR_STATUS_OK) {
        return Remember(index_status);
    }

    const std::size_t local_count = local_indices_.size();
    const blitzar_status capacity_status =
        local_count <= arena_.Count() && local_count <= particle_ids_.size()
            ? BLITZAR_STATUS_OK
            : BLITZAR_STATUS_INVALID_ARGUMENT;

    const blitzar_status synchronized_capacity_status =
        SynchronizeSimulationStatus(mpi_context_, capacity_status, "set-particles-capacity");

    if (synchronized_capacity_status != BLITZAR_STATUS_OK) {
        return Remember(synchronized_capacity_status);
    }

    SrvParticleCommitRequest commit_request{stage, local_indices_, arena_, particles_, accelerations_,
        workspace_, std::span<std::uint64_t>(particle_ids_), domain_, std::move(candidate_domain),
        local_particle_count_, source_particle_count_, exchange_buffer_, particles_ready_};

    const blitzar_status commit_status = SrvCommitStagedParticles(commit_request);

    return Remember(commit_status);
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
