#include "io/snap/codec/SnapshotV2Writer.hpp"

#include "io/snap/codec/SnapshotWire.hpp"

#include <algorithm>
#include <fstream>
#include <ios>
#include <new>
#include <span>
#include <system_error>

namespace blitzar_io {

namespace {

template <typename Value>
[[nodiscard]] bool WriteRange(SnapshotWireWriter& wire, std::span<const Value> values) noexcept
{
    for (const Value value : values) {
        if (!wire.Put(value)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool BeginSection(
    SnapshotWireWriter& wire, SnapshotV2Section section, std::uint32_t length) noexcept
{
    return wire.Put(static_cast<std::uint16_t>(section)) && wire.Put(length);
}

[[nodiscard]] bool WriteParticleSection(
    SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    const std::uint64_t count = state.ParticleCount();
    const std::uint32_t length = 8U + static_cast<std::uint32_t>(count) * 64U;

    if (!BeginSection(wire, SnapshotV2Section::ParticleState, length) || !wire.Put(count)) {
        return false;
    }

    const SnapshotV2Particles& particles = state.particles;

    return WriteRange(wire, particles.ids) && WriteRange(wire, particles.position_x) &&
           WriteRange(wire, particles.position_y) && WriteRange(wire, particles.position_z) &&
           WriteRange(wire, particles.velocity_x) && WriteRange(wire, particles.velocity_y) &&
           WriteRange(wire, particles.velocity_z) && WriteRange(wire, particles.mass);
}

[[nodiscard]] bool WriteIntegratorSection(
    SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    return BeginSection(wire, SnapshotV2Section::IntegratorState, 25U) &&
           wire.Put(state.integrator.step) && wire.Put(state.integrator.time) &&
           wire.Put(state.integrator.timestep) &&
           wire.Put(static_cast<std::uint8_t>(state.integrator.kind));
}

[[nodiscard]] bool WriteScalarSections(
    SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    if (!BeginSection(wire, SnapshotV2Section::RngState, 8U) || !wire.Put(state.rng.seed)) {
        return false;
    }

    if (!BeginSection(wire, SnapshotV2Section::UnitSystem, 24U)) {
        return false;
    }

    const blitzar_core::UnitSystem& units = state.units.units;

    return wire.Put(units.length_scale) && wire.Put(units.mass_scale) && wire.Put(units.time_scale);
}

[[nodiscard]] bool WritePolicySection(
    SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    const blitzar_core::BackendExecutionPolicy& cpu = state.math.settings.cpu;
    const blitzar_core::BackendExecutionPolicy& hip = state.math.settings.hip;
    const blitzar_core::BackendExecutionPolicy& mpi = state.math.settings.mpi;

    const auto fma = [](const blitzar_core::FmaPolicy policy) {
        return static_cast<std::uint8_t>(policy);
    };

    const auto reduction = [](const blitzar_core::ReductionPolicy policy) {
        return static_cast<std::uint8_t>(policy);
    };

    return BeginSection(wire, SnapshotV2Section::ExecutionMath, 8U) &&
           wire.Put(static_cast<std::uint8_t>(state.math.settings.mode)) &&
           wire.Put(fma(cpu.fma)) && wire.Put(reduction(cpu.reduction)) && wire.Put(fma(hip.fma)) &&
           wire.Put(reduction(hip.reduction)) && wire.Put(fma(mpi.fma)) &&
           wire.Put(reduction(mpi.reduction)) &&
           wire.Put(static_cast<std::uint8_t>(state.math.compensator));
}

[[nodiscard]] bool WriteIdentitySections(
    SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    if (!BeginSection(wire, SnapshotV2Section::BackendIdentity, 3U)) {
        return false;
    }

    if (!wire.Put(static_cast<std::uint8_t>(state.backend.backend)) ||
        !wire.Put(static_cast<std::uint8_t>(state.backend.device)) ||
        !wire.Put(static_cast<std::uint8_t>(state.backend.precision))) {
        return false;
    }

    const SnapshotV2Domain& domain = state.domain;

    return BeginSection(wire, SnapshotV2Section::GlobalDomain, 56U) && wire.Put(domain.minimum.x) &&
           wire.Put(domain.minimum.y) && wire.Put(domain.minimum.z) && wire.Put(domain.maximum.x) &&
           wire.Put(domain.maximum.y) && wire.Put(domain.maximum.z) &&
           wire.Put(domain.source_rank_count) && wire.Put(domain.decomposition_version);
}

[[nodiscard]] bool WriteOwnershipSection(
    SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    const std::uint64_t count = state.ParticleCount();
    const std::uint32_t length = 1U + 4U + 8U + static_cast<std::uint32_t>(count) * 4U;

    if (!BeginSection(wire, SnapshotV2Section::SourceOwnership, length)) {
        return false;
    }

    if (!wire.Put(static_cast<std::uint8_t>(state.ownership.kind)) ||
        !wire.Put(state.ownership.source_rank_count) || !wire.Put(count)) {
        return false;
    }

    return WriteRange(wire, state.ownership.source_rank);
}

[[nodiscard]] bool WriteSections(SnapshotWireWriter& wire, const SnapshotV2State& state) noexcept
{
    return WriteParticleSection(wire, state) && WriteIntegratorSection(wire, state) &&
           WriteScalarSections(wire, state) && WritePolicySection(wire, state) &&
           WriteIdentitySections(wire, state) && WriteOwnershipSection(wire, state);
}

[[nodiscard]] bool WriteFrame(
    std::ostream& output, const SnapshotV2State& state, std::uint16_t section_count) noexcept
{
    SnapshotWireWriter wire(output);

    if (!wire.PutUnhashed(blitzar_core::SnapshotMagic) || !wire.PutUnhashed(SnapshotV2Version) ||
        !wire.PutUnhashed(section_count)) {
        return false;
    }

    return WriteSections(wire, state) && wire.PutUnhashed(wire.Checksum());
}

[[nodiscard]] blitzar_status ValidateInput(
    const SnapshotV2State& state, std::size_t max_particle_count) noexcept
{
    if (state.ParticleCount() > max_particle_count ||
        state.ParticleCount() > blitzar_core::SnapshotMaxParticleCount) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return state.IsValid() ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace

SnapshotV2Writer::SnapshotV2Writer(std::size_t max_particle_count) noexcept
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount)))
{
}

blitzar_status SnapshotV2Writer::Write(
    const std::filesystem::path& path, const SnapshotV2State& state) const
{
    const blitzar_status input_status = ValidateInput(state, max_particle_count_);

    if (input_status != BLITZAR_STATUS_OK) {
        return input_status;
    }

    try {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);

        if (!output.is_open()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        if (!WriteFrame(output, state, 8U)) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        output.flush();

        return output ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::ios_base::failure&) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

blitzar_status SnapshotV2Writer::WriteAtomic(
    const std::filesystem::path& path, const SnapshotV2State& state) const
{
    try {
        const blitzar_status input_status = ValidateInput(state, max_particle_count_);

        if (input_status != BLITZAR_STATUS_OK) {
            return input_status;
        }

        std::error_code status_error;

        if (std::filesystem::exists(path, status_error) || status_error) {
            return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::filesystem::path temporary = path;

        temporary += ".tmp";

        const blitzar_status write_status = Write(temporary, state);

        if (write_status != BLITZAR_STATUS_OK) {
            std::error_code cleanup_error;

            std::filesystem::remove(temporary, cleanup_error);

            return write_status;
        }

        std::error_code rename_error;

        std::filesystem::rename(temporary, path, rename_error);

        if (rename_error) {
            std::error_code cleanup_error;

            std::filesystem::remove(temporary, cleanup_error);

            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::ios_base::failure&) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace blitzar_io
