#ifndef BLITZAR_PARALLEL_MPI_GHOST_TRANSPORT_HPP
#define BLITZAR_PARALLEL_MPI_GHOST_TRANSPORT_HPP

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiGhostExchange.hpp"
#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <span>

namespace blitzar_parallel {

class MpiPacketTransport;

class MpiGhostTransport final {
public:
    MpiGhostTransport(const MpiSession& session, const MpiCollectives& collectives,
        const MpiPacketTransport& packets) noexcept;

    [[nodiscard]] blitzar_status Prepare(MpiGhostExchange& exchange,
        std::size_t send_capacity, std::size_t receive_capacity) const noexcept;

    [[nodiscard]] blitzar_status Begin(
        std::span<const ParticlePacket> local, MpiGhostExchange& exchange) const noexcept;
    [[nodiscard]] blitzar_status Complete(
        MpiGhostExchange& exchange, PacketBuffer& ghosts) const noexcept;
    [[nodiscard]] bool IsActive(const MpiGhostExchange& exchange) const noexcept;
    void Abort(MpiGhostExchange& exchange) const noexcept;

private:
    friend class MpiGhostExchange;

    static void ClearExchange(MpiGhostExchange::Impl& state) noexcept;
    static void AbortExchange(MpiGhostExchange::Impl& state) noexcept;

    const MpiSession& session_;
    [[maybe_unused]] const MpiCollectives& collectives_;
    const MpiPacketTransport& packets_;
};

} // namespace blitzar_parallel

#endif
