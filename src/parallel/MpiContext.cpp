#include "parallel/MpiContext.hpp"

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiGhostTransport.hpp"
#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSession.hpp"

#include <new>

namespace blitzar_parallel {

struct MpiContext::Impl final {
    MpiSession session;
    MpiCollectives collectives;
    MpiPacketTransport packets;
    MpiGhostTransport ghosts;

    Impl() noexcept
        : session(),
          collectives(session),
          packets(session, collectives),
          ghosts(session, collectives)
    {
    }
};

MpiContext::MpiContext() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    } catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;
        return;
    }
    status_ = impl_->session.Status();
}

MpiContext::~MpiContext() noexcept = default;

bool MpiContext::IsUsable() const noexcept
{
    return impl_ != nullptr && status_ == BLITZAR_STATUS_OK;
}

bool MpiContext::IsDistributed() const noexcept
{
    return IsUsable() && impl_->session.IsDistributed();
}

int MpiContext::Rank() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->session.Rank();
}

int MpiContext::Size() const noexcept
{
    return impl_ == nullptr ? 1 : impl_->session.Size();
}

blitzar_status MpiContext::Status() const noexcept
{
    return status_;
}

blitzar_status MpiContext::PrepareCapacity(
    std::size_t packet_capacity,
    GhostExchange& exchange) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    const blitzar_status packet_status =
        impl_->packets.Prepare(packet_capacity);
    if (packet_status != BLITZAR_STATUS_OK) {
        return packet_status;
    }
    return impl_->ghosts.Prepare(exchange, packet_capacity);
}

blitzar_status MpiContext::SynchronizeStatus(
    blitzar_status local_status,
    const char* operation,
    const char* phase,
    blitzar_status& global_status) const noexcept
{
    if (impl_ == nullptr) {
        global_status = status_;
        return status_;
    }
    return impl_->collectives.SynchronizeStatus(
        local_status, operation, phase, global_status);
}

blitzar_status MpiContext::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum,
    std::span<blitzar_core::Scalar> maximum) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->collectives.ReduceBounds(minimum, maximum);
}

blitzar_status MpiContext::ReduceMax(
    int local_value, int& global_value) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->collectives.ReduceMax(local_value, global_value);
}

blitzar_status MpiContext::BeginGhostExchange(
    std::span<const ParticlePacket> local,
    GhostExchange& exchange) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->ghosts.Begin(local, exchange);
}

blitzar_status MpiContext::CompleteGhostExchange(
    GhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    if (impl_ == nullptr) {
        ghosts.Clear();
        return status_;
    }
    return impl_->ghosts.Complete(exchange, ghosts);
}

bool MpiContext::IsGhostExchangeActive(
    const GhostExchange& exchange) const noexcept
{
    return impl_ != nullptr && impl_->ghosts.IsActive(exchange);
}

void MpiContext::AbortGhostExchange(GhostExchange& exchange) const noexcept
{
    if (impl_ != nullptr) {
        impl_->ghosts.Abort(exchange);
    }
}

blitzar_status MpiContext::AllToAllCounts(
    std::span<const int> send_counts,
    std::span<int> receive_counts) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->packets.AllToAllCounts(send_counts, receive_counts);
}

blitzar_status MpiContext::AllToAllPackets(
    std::span<const ParticlePacket> send_packets,
    std::span<const int> send_counts,
    std::span<const int> send_displacements,
    std::span<ParticlePacket> receive_packets,
    std::span<const int> receive_counts,
    std::span<const int> receive_displacements) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->packets.AllToAllPackets(
        send_packets,
        send_counts,
        send_displacements,
        receive_packets,
        receive_counts,
        receive_displacements);
}

blitzar_status MpiContext::AllGatherCounts(
    int local_count, std::span<int> counts) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->packets.AllGatherCounts(local_count, counts);
}

blitzar_status MpiContext::AllGatherPackets(
    std::span<const ParticlePacket> local_packets,
    std::span<ParticlePacket> gathered_packets,
    std::span<const int> counts,
    std::span<const int> displacements) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }
    return impl_->packets.AllGatherPackets(
        local_packets, gathered_packets, counts, displacements);
}

}  // namespace blitzar_parallel
