#ifndef BLITZAR_PARALLEL_MPI_EXCHANGE_GHOST_MPI_GHOST_TRANSPORT_HPP
#define BLITZAR_PARALLEL_MPI_EXCHANGE_GHOST_MPI_GHOST_TRANSPORT_HPP

#include "parallel/mpi/collectives/MpiCollectives.hpp"
#include "parallel/mpi/exchange/ghost/MpiGhostExchange.hpp"
#include "parallel/mpi/exchange/packets/PacketWire.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <span>

namespace blitzar_parallel {

class MpiPacketTransport;

class MpiGhostTransport final {
public:
    MpiGhostTransport(const MpiSession& session, const MpiCollectives& collectives,
        const MpiPacketTransport& packets) noexcept;

    [[nodiscard]] blitzar_status Prepare(MpiGhostExchange& exchange, std::size_t send_capacity,
        std::size_t receive_capacity) const noexcept;

    [[nodiscard]] blitzar_status Begin(
        std::span<const ParticlePacket> local, MpiGhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status Complete(
        MpiGhostExchange& exchange, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] bool IsActive(const MpiGhostExchange& exchange) const noexcept;
    void Abort(MpiGhostExchange& exchange) const noexcept;

private:
    struct BeginLayout final {
        std::size_t chunk_packets{};
        std::size_t receive_slots{};
        std::size_t receive_request_count{};
        std::size_t send_request_count{};
        std::size_t receive_wire_size{};
        std::size_t send_wire_size{};
    };

    [[nodiscard]] blitzar_status PrepareLocal(std::span<const ParticlePacket> local,
        MpiGhostExchange::Impl& state, std::size_t& local_bytes) const noexcept;
    [[nodiscard]] blitzar_status GatherPeerCounts(
        std::size_t local_size, MpiGhostExchange::Impl& state) const noexcept;
    [[nodiscard]] blitzar_status PrepareLayout(std::size_t local_size, std::size_t local_bytes,
        MpiGhostExchange::Impl& state, BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PreparePeerLayout(std::size_t local_size, std::size_t local_bytes,
        MpiGhostExchange::Impl& state, BeginLayout& layout) const noexcept;
    [[nodiscard]] bool PreparePeerCapacity(
        std::size_t peer, MpiGhostExchange::Impl& state, BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PrepareLocalLayout(
        std::size_t local_size, std::size_t remote_peer_count, BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PrepareWireLayout(
        std::size_t local_bytes, std::size_t remote_peer_count, BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PrepareRoundStorage(
        MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PostRequests(std::span<const ParticlePacket> local,
        MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PostReceiveRequests(
        MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status PostSendRequests(std::span<const ParticlePacket> local,
        MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status WaitGhostRequests(MpiGhostExchange::Impl& state) const noexcept;
    [[nodiscard]] blitzar_status CompleteActive(
        MpiGhostExchange::Impl& state, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] blitzar_status CountReceived(
        MpiGhostExchange::Impl& state, std::size_t& total) const noexcept;
    [[nodiscard]] blitzar_status PrepareGhostBuffer(
        std::size_t total, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] blitzar_status DecodeReceived(
        MpiGhostExchange::Impl& state, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] blitzar_status PrepareStorage(MpiGhostExchange::Impl& state,
        std::size_t send_capacity, std::size_t receive_capacity) const noexcept;

    friend class MpiGhostExchange;

    static void ClearExchange(MpiGhostExchange::Impl& state) noexcept;
    static void AbortExchange(MpiGhostExchange::Impl& state) noexcept;

    const MpiSession& session_;
    [[maybe_unused]] const MpiCollectives& collectives_;
    [[maybe_unused]] const MpiPacketTransport& packets_;
};

} // namespace blitzar_parallel

#endif
