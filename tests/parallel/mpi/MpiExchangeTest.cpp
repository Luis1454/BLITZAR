#include "MpiCases.hpp"
#include "fixtures/ProtocolFixture.hpp"

#include <array>
#include <utility>

namespace blitzar_mpi_tests {

namespace {

bool ValidateStatusSynchronization(blitzar_parallel::MpiContext& context) noexcept
{
    blitzar_status global_status = BLITZAR_STATUS_OK;
    const blitzar_status local_status =
        context.Rank() == 0 ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_OK;

    return context.SynchronizeStatus(local_status, "MpiTest", "injected-failure", global_status) ==
               BLITZAR_STATUS_OK &&
           global_status == BLITZAR_STATUS_INTERNAL_ERROR;
}

bool ValidateUnpreparedGhostExchange(ProtocolFixture& fixture) noexcept
{
    blitzar_parallel::MpiContext::GhostExchange unprepared_exchange;
    const blitzar_status expected_unprepared_begin =
        fixture.context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (fixture.exchange.BeginGhosts(fixture.particles.State(), fixture.ids, unprepared_exchange) !=
            expected_unprepared_begin ||
        fixture.context.IsGhostExchangeActive(unprepared_exchange)) {
        return false;
    }

    return true;
}

bool PrepareGhostExchange(
    ProtocolFixture& fixture, blitzar_parallel::MpiContext::GhostExchange& exchange) noexcept
{
    if (fixture.context.PrepareCapacity(fixture.packet_capacity, exchange) != BLITZAR_STATUS_OK) {
        return false;
    }

    if (fixture.exchange.BeginGhosts(fixture.particles.State(), fixture.ids, exchange) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    if (fixture.context.IsDistributed() && !fixture.context.IsGhostExchangeActive(exchange)) {
        return false;
    }

    if (fixture.context.IsDistributed() && fixture.context.Rank() == 0) {
        fixture.context.AbortGhostExchange(exchange);
    }

    return true;
}

bool ValidateAbortedGhostExchange(
    ProtocolFixture& fixture, blitzar_parallel::MpiContext::GhostExchange& exchange) noexcept
{
    blitzar_parallel::PacketBuffer aborted_ghosts;

    aborted_ghosts.Reserve(1);
    aborted_ghosts.Resize(1);

    const blitzar_status aborted_completion_status =
        fixture.exchange.CompleteGhosts(exchange, aborted_ghosts);

    const blitzar_status expected_aborted_completion =
        fixture.context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (aborted_completion_status != expected_aborted_completion || aborted_ghosts.Size() != 0) {
        return false;
    }

    return true;
}

bool ValidateRecoveredGhostExchange(ProtocolFixture& fixture) noexcept
{
    blitzar_parallel::PacketBuffer recovered_ghosts;

    recovered_ghosts.Reserve(static_cast<std::size_t>(fixture.context.Size()));

    if (fixture.exchange.ExchangeGhosts(fixture.particles.State(), fixture.ids, recovered_ghosts) !=
            BLITZAR_STATUS_OK ||
        recovered_ghosts.Size() != static_cast<std::size_t>(fixture.context.Size() - 1)) {
        return false;
    }

    return true;
}

bool ValidateInitialGhostExchange(ProtocolFixture& fixture) noexcept
{
    if (!ValidateUnpreparedGhostExchange(fixture)) {
        return false;
    }

    blitzar_parallel::MpiContext::GhostExchange exchange;

    if (!PrepareGhostExchange(fixture, exchange) ||
        !ValidateAbortedGhostExchange(fixture, exchange)) {
        return false;
    }

    return ValidateRecoveredGhostExchange(fixture);
}

bool ValidateMovedGhostExchange(ProtocolFixture& fixture) noexcept
{
    blitzar_parallel::MpiContext::GhostExchange active_exchange;
    blitzar_parallel::MpiContext::GhostExchange replacement_exchange;

    if (fixture.context.PrepareCapacity(fixture.packet_capacity, active_exchange) !=
            BLITZAR_STATUS_OK ||
        fixture.context.PrepareCapacity(fixture.packet_capacity, replacement_exchange) !=
            BLITZAR_STATUS_OK ||
        fixture.exchange.BeginGhosts(fixture.particles.State(), fixture.ids, active_exchange) !=
            BLITZAR_STATUS_OK) {
        return false;
    }

    active_exchange = std::move(replacement_exchange);

    if (fixture.context.IsGhostExchangeActive(active_exchange) ||
        fixture.context.IsGhostExchangeActive(replacement_exchange)) {
        return false;
    }

    blitzar_parallel::PacketBuffer moved_ghosts;
    const blitzar_status moved_completion_status =
        fixture.exchange.CompleteGhosts(active_exchange, moved_ghosts);

    const blitzar_status expected_moved_completion =
        fixture.context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (moved_completion_status != expected_moved_completion || moved_ghosts.Size() != 0) {
        return false;
    }

    blitzar_parallel::PacketBuffer recovered_after_move;

    recovered_after_move.Reserve(static_cast<std::size_t>(fixture.context.Size()));

    if (fixture.exchange.ExchangeGhosts(
            fixture.particles.State(), fixture.ids, recovered_after_move) != BLITZAR_STATUS_OK ||
        recovered_after_move.Size() != static_cast<std::size_t>(fixture.context.Size() - 1)) {
        return false;
    }

    return true;
}

bool ValidateUninitializedMigration(ProtocolFixture& fixture) noexcept
{
    blitzar_parallel::DomainDecomposition uninitialized_domain;
    blitzar_parallel::MpiExchange uninitialized_exchange(
        fixture.context, uninitialized_domain, fixture.packet_capacity);

    blitzar_parallel::PacketBuffer uninitialized_received;

    return uninitialized_exchange.Migrate(fixture.particles.State(), fixture.ids,
               uninitialized_received) == BLITZAR_STATUS_INVALID_ARGUMENT &&
           uninitialized_received.Size() == 0;
}

} // namespace

bool RunErrorSynchronizationCase(blitzar_parallel::MpiContext& context) noexcept
{
    if (!ValidateStatusSynchronization(context)) {
        return false;
    }

    ProtocolFixture fixture(context);

    if (!fixture.Initialize() || !ValidateInitialGhostExchange(fixture) ||
        !ValidateMovedGhostExchange(fixture) || !ValidateInvalidGhostExchange(fixture)) {
        return false;
    }

    return ValidateUninitializedMigration(fixture);
}

} // namespace blitzar_mpi_tests
