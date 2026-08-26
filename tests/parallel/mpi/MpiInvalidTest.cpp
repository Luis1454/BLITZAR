#include "fixtures/ProtocolFixture.hpp"

#include <cmath>
#include <limits>
#include <span>

namespace blitzar_mpi_tests {

namespace {

bool BeginInvalidGhostExchange(ProtocolFixture& fixture,
    blitzar_core::ParticleStateView local_state, std::span<const std::uint64_t> local_ids,
    blitzar_parallel::MpiContext::GhostExchange& exchange) noexcept
{
    if (fixture.context.PrepareCapacity(fixture.packet_capacity, exchange) != BLITZAR_STATUS_OK) {
        return false;
    }

    return fixture.exchange.BeginGhosts(local_state, local_ids, exchange) ==
               BLITZAR_STATUS_INVALID_ARGUMENT &&
           !fixture.context.IsGhostExchangeActive(exchange);
}

bool ValidateInvalidCompletion(
    ProtocolFixture& fixture, blitzar_parallel::MpiContext::GhostExchange& exchange) noexcept
{
    blitzar_parallel::PacketBuffer ghosts;
    const blitzar_status invalid_ghost_completion_status =
        fixture.exchange.CompleteGhosts(exchange, ghosts);

    const blitzar_status expected_invalid_ghost_completion =
        fixture.context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    return invalid_ghost_completion_status == expected_invalid_ghost_completion &&
           ghosts.Size() == 0;
}

bool ValidateInvalidMigration(ProtocolFixture& fixture, blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids) noexcept
{
    blitzar_parallel::PacketBuffer received;

    received.Reserve(1);

    return fixture.exchange.Migrate(local_state, local_ids, received) ==
               BLITZAR_STATUS_INVALID_ARGUMENT &&
           received.Size() == 0;
}

bool ValidateEscapedMigration(ProtocolFixture& fixture) noexcept
{
    blitzar_particles::ParticleBuffer escaped(1);
    const blitzar_parallel::DomainBounds bounds = fixture.domain.GlobalBounds();
    const double escaped_x =
        fixture.context.Rank() == 0
            ? std::nextafter(bounds.maximum.x, std::numeric_limits<double>::infinity())
            : bounds.maximum.x;

    blitzar_parallel::PacketBuffer received;

    if (escaped.SetPosition(0, {escaped_x, 0.0, 0.0}) != BLITZAR_STATUS_OK) {
        return false;
    }

    return fixture.exchange.Migrate(escaped.State(), fixture.ids, received) ==
               BLITZAR_STATUS_INVALID_ARGUMENT &&
           received.Size() == 0;
}

} // namespace

bool ValidateInvalidGhostExchange(ProtocolFixture& fixture) noexcept
{
    blitzar_core::ParticleStateView invalid_state{};

    invalid_state.count = 1;
    invalid_state.source_count = 1;

    const blitzar_core::ParticleStateView local_state =
        fixture.context.Rank() == 0 ? invalid_state : fixture.particles.State();

    const std::span<const std::uint64_t> local_ids =
        fixture.context.Rank() == 0 ? std::span<const std::uint64_t>{}
                                    : std::span<const std::uint64_t>(fixture.ids);

    blitzar_parallel::MpiContext::GhostExchange exchange;

    if (!BeginInvalidGhostExchange(fixture, local_state, local_ids, exchange) ||
        !ValidateInvalidCompletion(fixture, exchange) ||
        !ValidateInvalidMigration(fixture, local_state, local_ids)) {
        return false;
    }

    return ValidateEscapedMigration(fixture);
}

} // namespace blitzar_mpi_tests
