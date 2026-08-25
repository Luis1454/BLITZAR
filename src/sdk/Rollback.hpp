#ifndef BLITZAR_SDK_ROLLBACK_HPP
#define BLITZAR_SDK_ROLLBACK_HPP

#include "parallel/MpiTypes.hpp"
#include "particles/ParticleArena.hpp"
#include "particles/ParticleBuffer.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_sdk {

struct ArenaCaptureRequest final {
    blitzar_particles::ParticleArena& arena;
    std::size_t local_count;
    std::span<const std::uint64_t> ids;
    blitzar_parallel::PacketBuffer& snapshot;
};

struct ArenaRestoreRequest final {
    const blitzar_parallel::PacketBuffer& snapshot;
    blitzar_particles::ParticleArena& arena;
    blitzar_particles::ParticleBuffer& particles;
    std::span<std::uint64_t> ids;
    std::size_t local_count;
};

[[nodiscard]] blitzar_status CaptureArenaState(ArenaCaptureRequest& request) noexcept;

[[nodiscard]] blitzar_status RestoreArenaState(ArenaRestoreRequest& request) noexcept;

[[nodiscard]] blitzar_status CaptureForceState(
    blitzar_core::ForceView force, blitzar_parallel::PacketBuffer& snapshot) noexcept;

[[nodiscard]] blitzar_status RestoreForceState(
    const blitzar_parallel::PacketBuffer& snapshot, blitzar_core::ForceView force) noexcept;

} // namespace blitzar_sdk

#endif
