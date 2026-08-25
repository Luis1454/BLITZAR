#ifndef BLITZAR_SDK_TRANSACTION_HPP
#define BLITZAR_SDK_TRANSACTION_HPP

#include "integration/KdkCheckpoint.hpp"
#include "particles/AccelerationBuffer.hpp"
#include "sdk/Rollback.hpp"

#include <cstdint>
#include <vector>

namespace blitzar_sdk {

struct TransactionState final {
    blitzar_particles::ParticleArena& arena;
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::AccelerationBuffer& accelerations;
    blitzar_integration::KdkCheckpoint& checkpoint;
    std::vector<std::uint64_t>& ids;
    std::size_t& local_count;
    blitzar_parallel::PacketBuffer& exchange;
    blitzar_parallel::PacketBuffer& arena_snapshot;
    blitzar_parallel::PacketBuffer& force_snapshot;
    blitzar_parallel::PacketBuffer& exchange_snapshot;
};

class StepTransaction final {
public:
    explicit StepTransaction(TransactionState state) noexcept;

    [[nodiscard]] blitzar_status Prepare() noexcept;
    void Begin() noexcept;
    void Complete() noexcept;
    void Commit() noexcept;
    void Abort() noexcept;

private:
    enum class Phase : std::uint8_t {
        Aborted,
        Prepared,
        InFlight,
        Complete,
        Committed,
    };

    void ResetSnapshots() noexcept;

    blitzar_particles::ParticleArena& arena_;
    blitzar_particles::ParticleBuffer& particles_;
    blitzar_particles::AccelerationBuffer& accelerations_;
    blitzar_integration::KdkCheckpoint& checkpoint_;
    std::vector<std::uint64_t>& ids_;
    std::size_t& local_count_;
    blitzar_parallel::PacketBuffer& exchange_;
    blitzar_parallel::PacketBuffer& arena_snapshot_;
    blitzar_parallel::PacketBuffer& force_snapshot_;
    blitzar_parallel::PacketBuffer& exchange_snapshot_;
    std::size_t local_count_before_{0};
    std::size_t acceleration_count_before_{0};
    std::size_t checkpoint_count_before_{0};
    Phase phase_{Phase::Aborted};
};

} // namespace blitzar_sdk

#endif
