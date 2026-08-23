#ifndef BLITZAR_PARALLEL_MPI_GHOST_TRANSPORT_HPP
#define BLITZAR_PARALLEL_MPI_GHOST_TRANSPORT_HPP

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiGhostExchange.hpp"
#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <span>

namespace blitzar_parallel {

class MpiGhostTransport final {
public:
    MpiGhostTransport(const MpiSession& session, const MpiCollectives& collectives) noexcept;

    [[nodiscard]] blitzar_status Prepare(
        MpiGhostExchange& exchange, std::size_t packet_capacity) const noexcept;

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
};

} // namespace blitzar_parallel

#endif
