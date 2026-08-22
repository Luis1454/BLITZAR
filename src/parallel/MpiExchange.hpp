#ifndef BLITZAR_PARALLEL_MPI_EXCHANGE_HPP
#define BLITZAR_PARALLEL_MPI_EXCHANGE_HPP

#include "core/Types.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/MpiTypes.hpp"

#include <cstdint>
#include <span>

namespace blitzar_parallel {

class MpiExchange final {
public:
    MpiExchange(
        const MpiContext& context,
        const DomainDecomposition& decomposition) noexcept;

    [[nodiscard]] blitzar_status ExchangeGhosts(
        blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids,
        PacketBuffer& ghosts) const noexcept;

    [[nodiscard]] blitzar_status BeginGhosts(
        blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids,
        MpiContext::GhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status CompleteGhosts(
        MpiContext::GhostExchange& exchange,
        PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] blitzar_status SynchronizeStatus(
        blitzar_status local_status, const char* phase) const noexcept;

    [[nodiscard]] blitzar_status Migrate(
        blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids,
        PacketBuffer& received) const noexcept;

    [[nodiscard]] blitzar_status Gather(
        blitzar_core::ParticleStateView local_state,
        std::span<const std::uint64_t> local_ids,
        PacketBuffer& gathered) const noexcept;

private:
    const MpiContext& context_;
    const DomainDecomposition& decomposition_;
};

}  // namespace blitzar_parallel

#endif
