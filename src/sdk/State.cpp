#include "sdk/State.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_sdk {

blitzar_core::ParticleStateView ParticleInputStage::State() const noexcept
{
    return {position_x.size(), std::span<const blitzar_core::Scalar>(position_x),
        std::span<const blitzar_core::Scalar>(position_y),
        std::span<const blitzar_core::Scalar>(position_z),
        std::span<const blitzar_core::Scalar>(velocity_x),
        std::span<const blitzar_core::Scalar>(velocity_y),
        std::span<const blitzar_core::Scalar>(velocity_z),
        std::span<const blitzar_core::Scalar>(mass), position_x.size()};
}

blitzar_status StageParticleInput(
    blitzar_core::ParticleStateView input, ParticleInputStage& stage) noexcept
{
    const std::size_t count = input.count;

    if (!blitzar_core::IsValid(input)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        stage.position_x.resize(count);
        stage.position_y.resize(count);
        stage.position_z.resize(count);
        stage.velocity_x.resize(count);
        stage.velocity_y.resize(count);
        stage.velocity_z.resize(count);
        stage.mass.resize(count);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    for (std::size_t index = 0; index < input.count; ++index) {
        if (!std::isfinite(input.x[index]) || !std::isfinite(input.y[index]) ||
            !std::isfinite(input.z[index]) || !std::isfinite(input.velocity_x[index]) ||
            !std::isfinite(input.velocity_y[index]) || !std::isfinite(input.velocity_z[index]) ||
            !std::isfinite(input.mass[index]) || input.mass[index] < 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        stage.position_x[index] = input.x[index];
        stage.position_y[index] = input.y[index];
        stage.position_z[index] = input.z[index];
        stage.velocity_x[index] = input.velocity_x[index];
        stage.velocity_y[index] = input.velocity_y[index];
        stage.velocity_z[index] = input.velocity_z[index];
        stage.mass[index] = input.mass[index];
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status SynchronizeSimulationStatus(
    const blitzar_parallel::MpiContext& context, blitzar_status local_status,
    const char* phase) noexcept
{
    if (!context.IsDistributed()) {
        return local_status;
    }

    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context.SynchronizeStatus(local_status, "Simulation", phase, global_status);

    return synchronization_status == BLITZAR_STATUS_OK ? global_status : synchronization_status;
}

blitzar_status StoreGhosts(
    blitzar_parallel::PacketBuffer& ghosts, blitzar_particles::SourceBuffer& source) noexcept
{
    if (!source.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (const blitzar_parallel::ParticlePacket& packet : ghosts.View()) {
        if (!std::isfinite(packet.x) || !std::isfinite(packet.y) ||
            !std::isfinite(packet.z) || !std::isfinite(packet.velocity_x) ||
            !std::isfinite(packet.velocity_y) || !std::isfinite(packet.velocity_z) ||
            !std::isfinite(packet.mass) || packet.mass < 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    if (ghosts.Size() > source.Capacity() ||
        source.SetCount(ghosts.Size()) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const auto position_x = source.PositionX();
    const auto position_y = source.PositionY();
    const auto position_z = source.PositionZ();
    const auto velocity_x = source.VelocityX();
    const auto velocity_y = source.VelocityY();
    const auto velocity_z = source.VelocityZ();
    const auto mass = source.Mass();

    for (std::size_t offset = 0; offset < ghosts.Size(); ++offset) {
        const blitzar_parallel::ParticlePacket& packet = ghosts.View()[offset];
        const std::size_t index = offset;

        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
    }

    ghosts.Clear();

    return BLITZAR_STATUS_OK;
}

blitzar_status StoreLocalPackets(PacketStoreRequest& request) noexcept
{
    if (request.packets.Size() > request.arena.Count() ||
        request.packets.Size() > request.ids.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (const blitzar_parallel::ParticlePacket& packet : request.packets.View()) {
        if (packet.id >= request.particle_count || !std::isfinite(packet.x) ||
            !std::isfinite(packet.y) || !std::isfinite(packet.z) ||
            !std::isfinite(packet.velocity_x) || !std::isfinite(packet.velocity_y) ||
            !std::isfinite(packet.velocity_z) || !std::isfinite(packet.mass) ||
            packet.mass < 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    const std::size_t count = request.packets.Size();
    const std::size_t previous_particle_count = request.particles.Count();
    const std::size_t previous_acceleration_count = request.accelerations.Count();
    const std::size_t previous_checkpoint_count = request.checkpoint.Count();

    if (request.particles.SetCount(count) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (request.accelerations.SetCount(count) != BLITZAR_STATUS_OK) {
        (void)request.particles.SetCount(previous_particle_count);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (request.checkpoint.SetCount(count) != BLITZAR_STATUS_OK) {
        (void)request.particles.SetCount(previous_particle_count);
        (void)request.accelerations.SetCount(previous_acceleration_count);
        (void)request.checkpoint.SetCount(previous_checkpoint_count);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const auto position_x = request.arena.PositionX();
    const auto position_y = request.arena.PositionY();
    const auto position_z = request.arena.PositionZ();
    const auto velocity_x = request.arena.VelocityX();
    const auto velocity_y = request.arena.VelocityY();
    const auto velocity_z = request.arena.VelocityZ();
    const auto mass = request.arena.Mass();

    for (std::size_t index = 0; index < request.packets.Size(); ++index) {
        const blitzar_parallel::ParticlePacket& packet = request.packets.View()[index];

        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
        request.ids[index] = packet.id;
    }

    request.local_count = count;

    return BLITZAR_STATUS_OK;
}

blitzar_status CopyPacketBuffer(
    const blitzar_parallel::PacketBuffer& source, blitzar_parallel::PacketBuffer& target) noexcept
{
    if (!target.ResizeBounded(source.Size())) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::copy(source.View().begin(), source.View().end(), target.View().begin());

    return BLITZAR_STATUS_OK;
}

blitzar_status CaptureArenaState(ArenaCaptureRequest& request) noexcept
{
    if (request.local_count > request.arena.Count() || request.local_count > request.ids.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!request.snapshot.ResizeBounded(request.local_count)) {
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
        request.local_count > request.arena.Count() ||
        request.local_count > request.ids.size()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (request.particles.SetCount(request.local_count) != BLITZAR_STATUS_OK) {
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
    if (!blitzar_core::IsValid(force)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!snapshot.ResizeBounded(force.count)) {
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

} // namespace blitzar_sdk
