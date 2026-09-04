#include "mpi/native/MpiNativeStatus.hpp"
#include "simulation/Sim.hpp"

#include <algorithm>
#include <cmath>
#include <span>

namespace blitzar_sim {

namespace {

[[nodiscard]] bool IsFinitePacket(const blitzar_parallel::ParticlePacket& packet) noexcept
{
    return std::isfinite(packet.x) && std::isfinite(packet.y) && std::isfinite(packet.z) &&
           std::isfinite(packet.velocity_x) && std::isfinite(packet.velocity_y) &&
           std::isfinite(packet.velocity_z) && std::isfinite(packet.mass) && packet.mass >= 0.0;
}

} // namespace

bool Sim::ValidateOutput(blitzar_core::ParticleOutputView output) const noexcept
{
    return particles_ready_ && output.count >= particle_count_ && blitzar_core::IsValid(output);
}

blitzar_status Sim::CopyLocalState(blitzar_core::ParticleOutputView output) const noexcept
{
    const blitzar_core::ParticleStateView state = particle_state_.Particles().State();

    std::copy_n(state.x.begin(), particle_count_, output.x.begin());
    std::copy_n(state.y.begin(), particle_count_, output.y.begin());
    std::copy_n(state.z.begin(), particle_count_, output.z.begin());
    std::copy_n(state.velocity_x.begin(), particle_count_, output.velocity_x.begin());
    std::copy_n(state.velocity_y.begin(), particle_count_, output.velocity_y.begin());
    std::copy_n(state.velocity_z.begin(), particle_count_, output.velocity_z.begin());
    std::copy_n(state.mass.begin(), particle_count_, output.mass.begin());

    return BLITZAR_STATUS_OK;
}

blitzar_status Sim::GatherState(blitzar_core::ParticleOutputView output) const noexcept
{
    const blitzar_status gather_status =
        runtime_.Exchange().Gather(particle_state_.Particles().State(),
            std::span<const std::uint64_t>(particle_ids_).first(local_particle_count_),
            gathered_buffer_);

    if (gather_status != BLITZAR_STATUS_OK || gathered_buffer_.Size() != particle_count_) {
        return gather_status == BLITZAR_STATUS_OK ? BLITZAR_STATUS_INTERNAL_ERROR : gather_status;
    }

    const std::span<blitzar_parallel::ParticlePacket> packets = gathered_buffer_.View();

    std::sort(packets.begin(), packets.end(),
        [](const auto& left, const auto& right) { return left.id < right.id; });

    for (std::size_t index = 0; index < packets.size(); ++index) {
        const blitzar_parallel::ParticlePacket& packet = packets[index];

        if (packet.id != index || !IsFinitePacket(packet)) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        output.x[index] = packet.x;
        output.y[index] = packet.y;
        output.z[index] = packet.z;
        output.velocity_x[index] = packet.velocity_x;
        output.velocity_y[index] = packet.velocity_y;
        output.velocity_z[index] = packet.velocity_z;
        output.mass[index] = packet.mass;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Sim::GetState(blitzar_core::ParticleOutputView output) const noexcept
{
    blitzar_status status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(),
        ValidateOutput(output) ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "get-state-preflight");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    const bool local_state_valid = particle_state_.Particles().Count() == local_particle_count_ &&
                                   local_particle_count_ <= particle_ids_.size() &&
                                   blitzar_core::IsValid(particle_state_.Particles().State());

    status = blitzar_parallel::SynchronizeStatus(runtime_.Mpi(),
        local_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "get-state-state");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    status = runtime_.Mpi().IsDistributed() ? GatherState(output) : CopyLocalState(output);

    return Remember(status);
}

blitzar_status Sim::GetLocalState(blitzar_core::ParticleOutputView output,
    std::span<std::uint64_t> ids, std::size_t& count) const noexcept
{
    count = 0;

    if (!runtime_.Mpi().IsUsable()) {
        return Remember(runtime_.Mpi().Status());
    }

    const bool output_valid = particles_ready_ && output.count >= local_particle_count_ &&
                              ids.size() >= local_particle_count_ &&
                              blitzar_core::IsValid(output) &&
                              particle_state_.Particles().Count() == local_particle_count_ &&
                              local_particle_count_ <= particle_ids_.size() &&
                              local_particle_count_ <= output_order_.size() &&
                              blitzar_core::IsValid(particle_state_.Particles().State());

    if (!output_valid) {
        return Remember(BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    const blitzar_core::ParticleStateView state = particle_state_.Particles().State();

    for (std::size_t index = 0; index < local_particle_count_; ++index) {
        output_order_[index] = index;
    }

    std::sort(output_order_.begin(),
        output_order_.begin() + static_cast<std::ptrdiff_t>(local_particle_count_),
        [this](std::size_t left, std::size_t right) {
            return particle_ids_[left] < particle_ids_[right];
        });

    for (std::size_t index = 0; index < local_particle_count_; ++index) {
        const std::size_t source = output_order_[index];

        if (source >= state.count || (index != 0U && particle_ids_[source] <= ids[index - 1U])) {
            return Remember(BLITZAR_STATUS_INTERNAL_ERROR);
        }

        ids[index] = particle_ids_[source];
        output.x[index] = state.x[source];
        output.y[index] = state.y[source];
        output.z[index] = state.z[source];
        output.velocity_x[index] = state.velocity_x[source];
        output.velocity_y[index] = state.velocity_y[source];
        output.velocity_z[index] = state.velocity_z[source];
        output.mass[index] = state.mass[source];
    }

    count = local_particle_count_;

    return Remember(BLITZAR_STATUS_OK);
}

} // namespace blitzar_sim
