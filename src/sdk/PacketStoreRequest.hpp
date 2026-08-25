#ifndef BLITZAR_SDK_PACKET_STORE_REQUEST_HPP
#define BLITZAR_SDK_PACKET_STORE_REQUEST_HPP

#include "integration/KdkCheckpoint.hpp"
#include "parallel/MpiTypes.hpp"
#include "particles/AccelerationBuffer.hpp"
#include "particles/ParticleArena.hpp"
#include "particles/ParticleBuffer.hpp"
#include "particles/SourceBuffer.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_sdk {

struct PacketStoreRequest final {
    blitzar_parallel::PacketBuffer& packets;
    blitzar_particles::ParticleArena& arena;
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::AccelerationBuffer& accelerations;
    blitzar_integration::KdkCheckpoint& checkpoint;
    std::span<std::uint64_t> ids;
    std::size_t particle_count;
    std::size_t& local_count;
};

[[nodiscard]] blitzar_status StoreGhosts(
    blitzar_parallel::PacketBuffer& ghosts, blitzar_particles::SourceBuffer& source) noexcept;

[[nodiscard]] blitzar_status StoreLocalPackets(PacketStoreRequest& request) noexcept;

[[nodiscard]] blitzar_status CopyPacketBuffer(
    const blitzar_parallel::PacketBuffer& source, blitzar_parallel::PacketBuffer& target) noexcept;

} // namespace blitzar_sdk

#endif
