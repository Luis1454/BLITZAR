#include "sdk/Transaction.hpp"

#include "sdk/PacketStoreRequest.hpp"

namespace blitzar_sdk {

StepTransaction::StepTransaction(TransactionState state) noexcept
    : arena_(state.arena), particles_(state.particles), accelerations_(state.accelerations),
      checkpoint_(state.checkpoint), ids_(state.ids), local_count_(state.local_count),
      exchange_(state.exchange), arena_snapshot_(state.arena_snapshot),
      force_snapshot_(state.force_snapshot), exchange_snapshot_(state.exchange_snapshot)
{
}

blitzar_status StepTransaction::Prepare() noexcept
{
    phase_ = Phase::Aborted;

    arena_snapshot_.Clear();
    force_snapshot_.Clear();
    exchange_snapshot_.Clear();

    local_count_before_ = particles_.Count();
    acceleration_count_before_ = accelerations_.Count();
    checkpoint_count_before_ = checkpoint_.Count();

    if (!arena_.IsValid() || !particles_.IsValid() || !accelerations_.IsValid() ||
        !checkpoint_.IsValid() || local_count_before_ != local_count_ ||
        local_count_before_ != acceleration_count_before_ ||
        local_count_before_ != checkpoint_count_before_ || local_count_before_ > arena_.Count() ||
        local_count_before_ > ids_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    ArenaCaptureRequest arena_request{
        arena_, local_count_before_, std::span<const std::uint64_t>(ids_), arena_snapshot_};

    blitzar_status status = CaptureArenaState(arena_request);

    if (status != BLITZAR_STATUS_OK) {
        ResetSnapshots();

        return status;
    }

    status = CaptureForceState(accelerations_.View(), force_snapshot_);

    if (status != BLITZAR_STATUS_OK) {
        ResetSnapshots();

        return status;
    }

    status = CopyPacketBuffer(exchange_, exchange_snapshot_);

    if (status != BLITZAR_STATUS_OK) {
        ResetSnapshots();

        return status;
    }

    phase_ = Phase::Prepared;

    return BLITZAR_STATUS_OK;
}

void StepTransaction::Begin() noexcept
{
    if (phase_ == Phase::Prepared) {
        phase_ = Phase::InFlight;
    }
}

void StepTransaction::Complete() noexcept
{
    if (phase_ == Phase::InFlight) {
        phase_ = Phase::Complete;
    }
}

void StepTransaction::Commit() noexcept
{
    if (phase_ == Phase::Complete) {
        ResetSnapshots();

        phase_ = Phase::Committed;
    }
}

void StepTransaction::Abort() noexcept
{
    if (phase_ == Phase::Committed || phase_ == Phase::Aborted) {
        return;
    }

    ArenaRestoreRequest arena_request{
        arena_snapshot_, arena_, particles_, std::span<std::uint64_t>(ids_), local_count_before_};

    (void)RestoreArenaState(arena_request);

    (void)accelerations_.SetCount(acceleration_count_before_);
    (void)checkpoint_.SetCount(checkpoint_count_before_);
    (void)RestoreForceState(force_snapshot_, accelerations_.View());

    local_count_ = local_count_before_;

    (void)checkpoint_.Capture(particles_.MutableView());
    (void)CopyPacketBuffer(exchange_snapshot_, exchange_);

    ResetSnapshots();

    phase_ = Phase::Aborted;
}

void StepTransaction::ResetSnapshots() noexcept
{
    arena_snapshot_.Clear();
    force_snapshot_.Clear();
    exchange_snapshot_.Clear();
}

} // namespace blitzar_sdk
