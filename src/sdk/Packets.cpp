#include "sdk/PacketStoreRequest.hpp"

#include <algorithm>
#include <cmath>

namespace blitzar_sdk {

namespace {

[[nodiscard]] bool IsFinitePacket(const blitzar_parallel::ParticlePacket& packet) noexcept
{
    return std::isfinite(packet.x) && std::isfinite(packet.y) && std::isfinite(packet.z) &&
           std::isfinite(packet.velocity_x) && std::isfinite(packet.velocity_y) &&
           std::isfinite(packet.velocity_z) && std::isfinite(packet.mass) && packet.mass >= 0.0;
}

struct CountSnapshot final {
    std::size_t particles{};
    std::size_t accelerations{};
    std::size_t checkpoint{};
};

[[nodiscard]] blitzar_status SetLocalCounts(
    PacketStoreRequest& request, std::size_t count, CountSnapshot previous) noexcept
{
    if (request.particles.SetCount(count) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (request.accelerations.SetCount(count) != BLITZAR_STATUS_OK) {
        (void)request.particles.SetCount(previous.particles);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    if (request.checkpoint.SetCount(count) != BLITZAR_STATUS_OK) {
        (void)request.particles.SetCount(previous.particles);
        (void)request.accelerations.SetCount(previous.accelerations);
        (void)request.checkpoint.SetCount(previous.checkpoint);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
}

void WriteGhosts(
    const blitzar_parallel::PacketBuffer& ghosts, blitzar_particles::SourceBuffer& source) noexcept
{
    const auto position_x = source.PositionX();
    const auto position_y = source.PositionY();
    const auto position_z = source.PositionZ();
    const auto velocity_x = source.VelocityX();
    const auto velocity_y = source.VelocityY();
    const auto velocity_z = source.VelocityZ();
    const auto mass = source.Mass();

    for (std::size_t index = 0; index < ghosts.Size(); ++index) {
        const blitzar_parallel::ParticlePacket& packet = ghosts.View()[index];

        position_x[index] = packet.x;
        position_y[index] = packet.y;
        position_z[index] = packet.z;
        velocity_x[index] = packet.velocity_x;
        velocity_y[index] = packet.velocity_y;
        velocity_z[index] = packet.velocity_z;
        mass[index] = packet.mass;
    }
}

void WriteLocalPackets(PacketStoreRequest& request) noexcept
{
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
}

} // namespace

blitzar_status StoreGhosts(
    blitzar_parallel::PacketBuffer& ghosts, blitzar_particles::SourceBuffer& source) noexcept
{
    if (!source.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (const blitzar_parallel::ParticlePacket& packet : ghosts.View()) {
        if (!IsFinitePacket(packet)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    if (ghosts.Size() > source.Capacity()) {
        const blitzar_status reserve_status = source.Reserve(ghosts.Size());

        if (reserve_status != BLITZAR_STATUS_OK) {
            return reserve_status;
        }
    }
    if (source.SetCount(ghosts.Size()) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    WriteGhosts(ghosts, source);
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
        if (packet.id >= request.particle_count || !IsFinitePacket(packet)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    const CountSnapshot previous{
        request.particles.Count(), request.accelerations.Count(), request.checkpoint.Count()};

    const std::size_t count = request.packets.Size();
    const blitzar_status count_status = SetLocalCounts(request, count, previous);

    if (count_status != BLITZAR_STATUS_OK) {
        return count_status;
    }

    WriteLocalPackets(request);

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

} // namespace blitzar_sdk
