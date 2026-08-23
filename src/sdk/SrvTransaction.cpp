#include "sdk/SrvTransaction.hpp"

namespace blitzar_sdk {

SrvStepTransaction::SrvStepTransaction(SrvTransactionState state) noexcept
    : arena_(state.arena), particles_(state.particles), accelerations_(state.accelerations),
      workspace_(state.workspace), ids_(state.ids), local_count_(state.local_count),
      source_count_(state.source_count), exchange_(state.exchange),
      arena_snapshot_(state.arena_snapshot), force_snapshot_(state.force_snapshot),
      exchange_snapshot_(state.exchange_snapshot)
{
}

blitzar_status SrvStepTransaction::Prepare() noexcept
{
    phase_ = Phase::Aborted;
    arena_snapshot_.Clear();
    force_snapshot_.Clear();
    exchange_snapshot_.Clear();
    local_count_before_ = particles_.Count();
    source_count_before_ = source_count_;
    acceleration_count_before_ = accelerations_.Count();
    workspace_count_before_ = workspace_.Count();

    if (!arena_.IsValid() || !particles_.IsValid() || !accelerations_.IsValid() ||
        !workspace_.IsValid() || local_count_before_ != local_count_ ||
        local_count_before_ != acceleration_count_before_ ||
        local_count_before_ != workspace_count_before_ ||
        local_count_before_ > source_count_before_ || source_count_before_ > arena_.Count() ||
        local_count_before_ > ids_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    SrvArenaCaptureRequest arena_request{
        arena_, local_count_before_, source_count_before_, ids_, arena_snapshot_};
    blitzar_status status = SrvCaptureArenaState(arena_request);

    if (status != BLITZAR_STATUS_OK) {
        ResetSnapshots();

        return status;
    }

    status = SrvCaptureForceState(accelerations_.View(), force_snapshot_);

    if (status != BLITZAR_STATUS_OK) {
        ResetSnapshots();

        return status;
    }

    status = SrvCopyPacketBuffer(exchange_, exchange_snapshot_);

    if (status != BLITZAR_STATUS_OK) {
        ResetSnapshots();

        return status;
    }

    phase_ = Phase::Prepared;

    return BLITZAR_STATUS_OK;
}

void SrvStepTransaction::Begin() noexcept
{
    if (phase_ == Phase::Prepared) {
        phase_ = Phase::InFlight;
    }
}

void SrvStepTransaction::Complete() noexcept
{
    if (phase_ == Phase::InFlight) {
        phase_ = Phase::Complete;
    }
}

void SrvStepTransaction::Commit() noexcept
{
    if (phase_ == Phase::Complete) {
        ResetSnapshots();
        phase_ = Phase::Committed;
    }
}

void SrvStepTransaction::Abort() noexcept
{
    if (phase_ == Phase::Committed || phase_ == Phase::Aborted) {
        return;
    }

    SrvArenaRestoreRequest arena_request{arena_snapshot_, arena_, particles_, ids_,
        local_count_before_, source_count_before_};
    (void)SrvRestoreArenaState(arena_request);

    (void)accelerations_.SetCount(acceleration_count_before_);
    (void)workspace_.SetCount(workspace_count_before_);
    (void)SrvRestoreForceState(force_snapshot_, accelerations_.View());

    local_count_ = local_count_before_;
    source_count_ = source_count_before_;

    (void)workspace_.Capture(particles_.MutableView());
    (void)SrvCopyPacketBuffer(exchange_snapshot_, exchange_);

    ResetSnapshots();
    phase_ = Phase::Aborted;
}

void SrvStepTransaction::ResetSnapshots() noexcept
{
    arena_snapshot_.Clear();
    force_snapshot_.Clear();
    exchange_snapshot_.Clear();
}

} // namespace blitzar_sdk
