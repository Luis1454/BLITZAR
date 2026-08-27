#ifndef BLITZAR_SIMULATION_STEP_SIM_PACKET_STORE_REQUEST_HPP
#define BLITZAR_SIMULATION_STEP_SIM_PACKET_STORE_REQUEST_HPP

#include "integration/kdk/KdkCheckpoint.hpp"
#include "mpi/packets/MpiPacketWire.hpp"
#include "particles/arena/ParticleArena.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "particles/source/ParticleSourceBuffer.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_sim {

struct SimPacketStoreRequest final {
    blitzar_parallel::PacketBuffer& packets;
    blitzar_particles::ParticleArena& arena;
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::ParticleAccelerationBuffer& accelerations;
    blitzar_integration::KdkCheckpoint& checkpoint;
    std::span<std::uint64_t> ids;
    std::size_t particle_count;
    std::size_t& local_count;
};

[[nodiscard]] blitzar_status StoreGhosts(blitzar_parallel::PacketBuffer& ghosts,
    blitzar_particles::ParticleSourceBuffer& source) noexcept;

[[nodiscard]] blitzar_status StoreLocalPackets(SimPacketStoreRequest& request) noexcept;

[[nodiscard]] blitzar_status CopyPacketBuffer(
    const blitzar_parallel::PacketBuffer& source, blitzar_parallel::PacketBuffer& target) noexcept;

} // namespace blitzar_sim

#endif
