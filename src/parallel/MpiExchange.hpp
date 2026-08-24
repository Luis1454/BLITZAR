#ifndef BLITZAR_PARALLEL_MPI_EXCHANGE_HPP
#define BLITZAR_PARALLEL_MPI_EXCHANGE_HPP

#include "core/Types.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/MpiTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_parallel {

struct MpiExchangeWorkspace final {
    MpiExchangeWorkspace(std::size_t packet_capacity, std::size_t peer_count);

    std::size_t packet_capacity{};
    PacketBuffer local_packets;
    PacketBuffer ordered_packets;
    std::vector<int> send_counts;
    std::vector<int> receive_counts;
    std::vector<int> send_displacements;
    std::vector<int> receive_displacements;
    std::vector<int> gather_counts;
    std::vector<int> gather_displacements;
    std::vector<std::size_t> send_offsets;
    std::vector<std::size_t> receive_offsets;
    std::vector<std::size_t> write_offsets;
};

class MpiExchange final {
public:
    MpiExchange(const MpiContext& context, const DomainDecomposition& decomposition,
        std::size_t packet_capacity = 0, std::size_t ghost_capacity = 0);

    [[nodiscard]] blitzar_status CapacityStatus() const noexcept
    {
        return capacity_status_;
    }

    [[nodiscard]] MpiContext::GhostExchange& PersistentGhostExchange() const noexcept;

    [[nodiscard]] blitzar_status ExchangeGhosts(blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids, PacketBuffer& ghosts) const noexcept;

    [[nodiscard]] blitzar_status BeginGhosts(blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids,
        MpiContext::GhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status CompleteGhosts(
        MpiContext::GhostExchange& exchange, PacketBuffer& ghosts) const noexcept;
    void AbortGhosts(MpiContext::GhostExchange& exchange, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] blitzar_status SynchronizeStatus(
        blitzar_status local_status, const char* phase) const noexcept;

    [[nodiscard]] blitzar_status Migrate(blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids, PacketBuffer& received) const noexcept;

    [[nodiscard]] blitzar_status Gather(blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids, PacketBuffer& gathered) const noexcept;

private:
    const MpiContext& context_;
    const DomainDecomposition& decomposition_;
    mutable MpiExchangeWorkspace workspace_;
    mutable MpiContext::GhostExchange ghost_exchange_;
    blitzar_status capacity_status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_parallel

#endif
