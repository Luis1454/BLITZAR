#include "sdk/Simulation.hpp"
#include "sdk/State.hpp"

#include <algorithm>
#include <cmath>
#include <span>

namespace blitzar_sdk {

namespace {

[[nodiscard]] bool IsFinitePacket(const blitzar_parallel::ParticlePacket& packet) noexcept
{
    return std::isfinite(packet.x) && std::isfinite(packet.y) && std::isfinite(packet.z) &&
           std::isfinite(packet.velocity_x) && std::isfinite(packet.velocity_y) &&
           std::isfinite(packet.velocity_z) && std::isfinite(packet.mass) && packet.mass >= 0.0;
}

} // namespace

bool Simulation::ValidateOutput(blitzar_core::ParticleOutputView output) const noexcept
{
    return particles_ready_ && output.count >= particle_count_ && blitzar_core::IsValid(output);
}

blitzar_status Simulation::CopyLocalState(blitzar_core::ParticleOutputView output) const noexcept
{
    const blitzar_core::ParticleStateView state = particles_.State();

    std::copy_n(state.x.begin(), particle_count_, output.x.begin());
    std::copy_n(state.y.begin(), particle_count_, output.y.begin());
    std::copy_n(state.z.begin(), particle_count_, output.z.begin());
    std::copy_n(state.velocity_x.begin(), particle_count_, output.velocity_x.begin());
    std::copy_n(state.velocity_y.begin(), particle_count_, output.velocity_y.begin());
    std::copy_n(state.velocity_z.begin(), particle_count_, output.velocity_z.begin());
    std::copy_n(state.mass.begin(), particle_count_, output.mass.begin());

    return BLITZAR_STATUS_OK;
}

blitzar_status Simulation::GatherState(blitzar_core::ParticleOutputView output) const noexcept
{
    const blitzar_status gather_status = mpi_exchange_.Gather(particles_.State(),
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

blitzar_status Simulation::GetState(blitzar_core::ParticleOutputView output) const noexcept
{
    blitzar_status status = SynchronizeSimulationStatus(mpi_context_,
        ValidateOutput(output) ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "get-state-preflight");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    const bool local_state_valid = particles_.Count() == local_particle_count_ &&
                                   local_particle_count_ <= particle_ids_.size() &&
                                   blitzar_core::IsValid(particles_.State());

    status = SynchronizeSimulationStatus(mpi_context_,
        local_state_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR, "get-state-state");

    if (status != BLITZAR_STATUS_OK) {
        return Remember(status);
    }

    status = mpi_context_.IsDistributed() ? GatherState(output) : CopyLocalState(output);

    return Remember(status);
}

} // namespace blitzar_sdk
