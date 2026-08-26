#ifndef BLITZAR_TESTS_FIXTURES_PROTOCOL_FIXTURE_HPP
#define BLITZAR_TESTS_FIXTURES_PROTOCOL_FIXTURE_HPP

#include "parallel/mpi/domain/DomainDecomposition.hpp"
#include "parallel/mpi/exchange/MpiExchange.hpp"
#include "particles/buffers/ParticleBuffer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace blitzar_mpi_tests {

struct ProtocolFixture final {
    blitzar_parallel::MpiContext& context;
    blitzar_particles::ParticleBuffer particles;
    blitzar_parallel::DomainDecomposition domain;
    std::size_t packet_capacity;
    blitzar_parallel::MpiExchange exchange;
    std::array<std::uint64_t, 1> ids{0};

    explicit ProtocolFixture(blitzar_parallel::MpiContext& value)
        : context(value), particles(1), packet_capacity(static_cast<std::size_t>(context.Size())),
          exchange(context, domain, packet_capacity)
    {
    }

    bool Initialize() noexcept
    {
        if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||

            particles.SetMass(0, 1.0) != BLITZAR_STATUS_OK) {
            return false;
        }

        return domain.Initialize(particles.State(), context) == BLITZAR_STATUS_OK;
    }
};

[[nodiscard]] bool ValidateInvalidGhostExchange(ProtocolFixture& fixture) noexcept;

} // namespace blitzar_mpi_tests

#endif
