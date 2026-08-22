#ifndef BLITZAR_PARALLEL_MPI_CONTEXT_HPP
#define BLITZAR_PARALLEL_MPI_CONTEXT_HPP

#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace blitzar_parallel {

class MpiContext final {
public:
    class GhostExchange final {
    public:
        GhostExchange() noexcept;
        ~GhostExchange() noexcept;

        GhostExchange(const GhostExchange&) = delete;
        GhostExchange& operator=(const GhostExchange&) = delete;
        GhostExchange(GhostExchange&& other) noexcept;
        GhostExchange& operator=(GhostExchange&& other) noexcept;

    private:
        friend class MpiContext;
        struct Impl;

        std::unique_ptr<Impl> impl_;
    };

    MpiContext() noexcept;
    ~MpiContext() noexcept;

    MpiContext(const MpiContext&) = delete;
    MpiContext& operator=(const MpiContext&) = delete;
    MpiContext(MpiContext&&) = delete;
    MpiContext& operator=(MpiContext&&) = delete;

    [[nodiscard]] bool IsUsable() const noexcept;
    [[nodiscard]] bool IsDistributed() const noexcept;
    [[nodiscard]] int Rank() const noexcept;
    [[nodiscard]] int Size() const noexcept;
    [[nodiscard]] blitzar_status Status() const noexcept;

    [[nodiscard]] blitzar_status ReduceBounds(
        std::span<blitzar_core::Scalar> minimum,
        std::span<blitzar_core::Scalar> maximum) const noexcept;
    [[nodiscard]] blitzar_status ReduceMax(
        int local_value, int& global_value) const noexcept;

    [[nodiscard]] blitzar_status BeginGhostExchange(
        std::span<const ParticlePacket> local,
        GhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status CompleteGhostExchange(
        GhostExchange& exchange, PacketBuffer& ghosts) const noexcept;

    [[nodiscard]] blitzar_status AllToAllCounts(
        std::span<const int> send_counts,
        std::span<int> receive_counts) const noexcept;
    [[nodiscard]] blitzar_status AllToAllPackets(
        std::span<const ParticlePacket> send_packets,
        std::span<const int> send_counts,
        std::span<const int> send_displacements,
        std::span<ParticlePacket> receive_packets,
        std::span<const int> receive_counts,
        std::span<const int> receive_displacements) const noexcept;
    [[nodiscard]] blitzar_status AllGatherCounts(
        int local_count, std::span<int> counts) const noexcept;
    [[nodiscard]] blitzar_status AllGatherPackets(
        std::span<const ParticlePacket> local_packets,
        std::span<ParticlePacket> gathered_packets,
        std::span<const int> counts,
        std::span<const int> displacements) const noexcept;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
    int rank_{0};
    int size_{1};
    blitzar_status status_{BLITZAR_STATUS_OK};
};

}  // namespace blitzar_parallel

#endif
