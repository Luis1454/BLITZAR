#ifndef BLITZAR_PARALLEL_MPI_CONTEXT_HPP
#define BLITZAR_PARALLEL_MPI_CONTEXT_HPP

#include "parallel/MpiGhostExchange.hpp"
#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace blitzar_parallel {

class MpiContext final {
public:
    using GhostExchange = MpiGhostExchange;

    struct GhostCapacity final {
        std::size_t send{};
        std::size_t receive{};
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
    [[nodiscard]] blitzar_status PrepareCapacity(
        std::size_t packet_capacity, GhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status PrepareCapacity(std::size_t packet_capacity,
        GhostExchange& exchange, GhostCapacity ghost_capacity) const noexcept;
    [[nodiscard]] blitzar_status SynchronizeStatus(blitzar_status local_status,
        std::string_view operation, std::string_view phase,
        blitzar_status& global_status) const noexcept;

    [[nodiscard]] blitzar_status ReduceBounds(std::span<blitzar_core::Scalar> minimum,
        std::span<blitzar_core::Scalar> maximum) const noexcept;
    [[nodiscard]] blitzar_status ReduceMax(int local_value, int& global_value) const noexcept;
    [[nodiscard]] blitzar_status Broadcast(
        std::span<blitzar_core::Scalar> values, int root) const noexcept;
    [[nodiscard]] blitzar_status Broadcast(
        std::span<std::uint64_t> values, int root) const noexcept;

    [[nodiscard]] blitzar_status BeginGhostExchange(
        std::span<const ParticlePacket> local, GhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status CompleteGhostExchange(
        GhostExchange& exchange, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] bool IsGhostExchangeActive(const GhostExchange& exchange) const noexcept;
    void AbortGhostExchange(GhostExchange& exchange) const noexcept;

    [[nodiscard]] blitzar_status AllToAllCounts(
        std::span<const int> send_counts, std::span<int> receive_counts) const noexcept;
    [[nodiscard]] blitzar_status AllToAllPackets(
        const AllToAllPacketRequest& request) const noexcept;
    [[nodiscard]] blitzar_status AllGatherCounts(
        int local_count, std::span<int> counts) const noexcept;
    [[nodiscard]] blitzar_status AllGatherPackets(std::span<const ParticlePacket> local_packets,
        std::span<ParticlePacket> gathered_packets, std::span<const int> counts,
        std::span<const int> displacements) const noexcept;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
    blitzar_status status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_parallel

#endif
