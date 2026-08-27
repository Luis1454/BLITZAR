#include "simulation/transaction/SimRollback.hpp"

namespace blitzar_sim {

blitzar_status CaptureArenaState(ArenaCaptureRequest& request) noexcept
{
    if (request.local_count > request.arena.Count() || request.local_count > request.ids.size() ||
        !request.snapshot.ResizeBounded(request.local_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const auto position_x = request.arena.PositionX();
    const auto position_y = request.arena.PositionY();
    const auto position_z = request.arena.PositionZ();
    const auto velocity_x = request.arena.VelocityX();
    const auto velocity_y = request.arena.VelocityY();
    const auto velocity_z = request.arena.VelocityZ();
    const auto mass = request.arena.Mass();

    for (std::size_t index = 0; index < request.local_count; ++index) {
        request.snapshot.View()[index] = {request.ids[index], position_x[index], position_y[index],
            position_z[index], velocity_x[index], velocity_y[index], velocity_z[index],
            mass[index]};
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status RestoreArenaState(ArenaRestoreRequest& request) noexcept
{
    if (request.snapshot.Size() != request.local_count ||
        request.local_count > request.arena.Count() || request.local_count > request.ids.size() ||
        request.particles.SetCount(request.local_count) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const auto position_x = request.arena.PositionX();
    const auto position_y = request.arena.PositionY();
    const auto position_z = request.arena.PositionZ();
    const auto velocity_x = request.arena.VelocityX();
    const auto velocity_y = request.arena.VelocityY();
    const auto velocity_z = request.arena.VelocityZ();
    const auto mass = request.arena.Mass();

    for (std::size_t index = 0; index < request.local_count; ++index) {
        const blitzar_parallel::ParticlePacket& packet = request.snapshot.View()[index];

        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
        request.ids[index] = packet.id;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status CaptureForceState(
    blitzar_core::ForceView force, blitzar_parallel::PacketBuffer& snapshot) noexcept
{
    if (!blitzar_core::IsValid(force) || !snapshot.ResizeBounded(force.count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < force.count; ++index) {
        snapshot.View()[index] = {
            0, force.x[index], force.y[index], force.z[index], 0.0, 0.0, 0.0, 0.0};
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status RestoreForceState(
    const blitzar_parallel::PacketBuffer& snapshot, blitzar_core::ForceView force) noexcept
{
    if (!blitzar_core::IsValid(force) || snapshot.Size() != force.count) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    for (std::size_t index = 0; index < force.count; ++index) {
        const blitzar_parallel::ParticlePacket& packet = snapshot.View()[index];

        force.x[index] = packet.x;
        force.y[index] = packet.y;
        force.z[index] = packet.z;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
