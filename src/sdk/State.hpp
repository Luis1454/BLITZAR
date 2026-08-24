#ifndef BLITZAR_SDK_STATE_HPP
#define BLITZAR_SDK_STATE_HPP

#include "integration/KdkCheckpoint.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/MpiTypes.hpp"
#include "particles/ParticleArena.hpp"
#include "particles/ParticleBuffer.hpp"
#include "particles/SourceBuffer.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_sdk {

struct ParticleInputStage final {
    std::vector<blitzar_core::Scalar> position_x;
    std::vector<blitzar_core::Scalar> position_y;
    std::vector<blitzar_core::Scalar> position_z;
    std::vector<blitzar_core::Scalar> velocity_x;
    std::vector<blitzar_core::Scalar> velocity_y;
    std::vector<blitzar_core::Scalar> velocity_z;
    std::vector<blitzar_core::Scalar> mass;

    [[nodiscard]] blitzar_core::ParticleStateView State() const noexcept;
};

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

[[nodiscard]] blitzar_status StageParticleInput(
    blitzar_core::ParticleStateView input, ParticleInputStage& stage) noexcept;

[[nodiscard]] blitzar_status SynchronizeSimulationStatus(
    const blitzar_parallel::MpiContext& context, blitzar_status local_status,
    const char* phase) noexcept;

[[nodiscard]] blitzar_status StoreGhosts(
    blitzar_parallel::PacketBuffer& ghosts, blitzar_particles::SourceBuffer& source) noexcept;

[[nodiscard]] blitzar_status StoreLocalPackets(PacketStoreRequest& request) noexcept;

[[nodiscard]] blitzar_status CopyPacketBuffer(
    const blitzar_parallel::PacketBuffer& source, blitzar_parallel::PacketBuffer& target) noexcept;

[[nodiscard]] blitzar_status CaptureArenaState(ArenaCaptureRequest& request) noexcept;

[[nodiscard]] blitzar_status RestoreArenaState(ArenaRestoreRequest& request) noexcept;

[[nodiscard]] blitzar_status CaptureForceState(
    blitzar_core::ForceView force, blitzar_parallel::PacketBuffer& snapshot) noexcept;

[[nodiscard]] blitzar_status RestoreForceState(
    const blitzar_parallel::PacketBuffer& snapshot, blitzar_core::ForceView force) noexcept;

} // namespace blitzar_sdk

#endif
