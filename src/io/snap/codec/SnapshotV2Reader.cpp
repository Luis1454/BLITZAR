#include "io/snap/codec/SnapshotV2Reader.hpp"

#include "io/snap/codec/SnapshotWire.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <new>
#include <span>
#include <system_error>
#include <vector>

namespace blitzar_io {

namespace {

constexpr std::size_t ParticleSectionFixedBytes = 8;
constexpr std::size_t ParticleSectionBytesPerParticle = 64;
constexpr std::size_t IntegratorSectionBytes = 25;
constexpr std::size_t RngSectionBytes = 8;
constexpr std::size_t UnitSectionBytes = 24;
constexpr std::size_t MathSectionBytes = 8;
constexpr std::size_t BackendSectionBytes = 3;
constexpr std::size_t DomainSectionBytes = 56;
constexpr std::size_t OwnershipSectionFixedBytes = 13;

struct SectionExpectation final {
    SnapshotV2Section section;
    std::size_t length;
};

template <typename Value>
[[nodiscard]] bool ReadRange(SnapshotWireReader& wire, std::span<Value> values) noexcept
{
    for (Value& value : values) {
        if (!wire.Read(value)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] blitzar_status ReadSectionTag(SnapshotWireReader& wire, SnapshotV2Section expected,
    std::size_t expected_length, std::size_t& declared_length) noexcept
{
    std::uint16_t tag{};
    std::uint32_t declared{};

    if (!wire.Read(tag) || !wire.Read(declared)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (static_cast<SnapshotV2Section>(tag) != expected) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    declared_length = declared;

    if (expected_length == 0U || declared_length == expected_length) {
        return BLITZAR_STATUS_OK;
    }

    return BLITZAR_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] bool ReadParticles(
    SnapshotWireReader& wire, SnapshotV2Target& target, std::uint64_t count) noexcept
{
    std::uint64_t stored_count{};

    if (!wire.Read(stored_count) || stored_count != count) {
        return false;
    }

    const std::size_t size = static_cast<std::size_t>(count);

    return ReadRange(wire, std::span<std::uint64_t>(target.ids).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.position_x).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.position_y).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.position_z).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.velocity_x).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.velocity_y).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.velocity_z).first(size)) &&
           ReadRange(wire, std::span<blitzar_core::Scalar>(target.mass).first(size));
}

[[nodiscard]] bool ReadIntegratorRngUnits(
    SnapshotWireReader& wire, SnapshotV2Target& target) noexcept
{
    SnapshotV2Integrator& integrator = target.integrator;

    std::uint8_t kind{};
    std::size_t declared{};

    if (!wire.Read(integrator.step) || !wire.Read(integrator.time) ||
        !wire.Read(integrator.timestep) || !wire.Read(kind)) {
        return false;
    }

    integrator.kind = static_cast<SnapshotV2IntegratorKind>(kind);

    if (ReadSectionTag(wire, SnapshotV2Section::RngState, RngSectionBytes, declared) !=
            BLITZAR_STATUS_OK ||
        !wire.Read(target.rng.seed)) {
        return false;
    }

    if (ReadSectionTag(wire, SnapshotV2Section::UnitSystem, UnitSectionBytes, declared) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_core::UnitSystem& units = target.units.units;

    return wire.Read(units.length_scale) && wire.Read(units.mass_scale) &&
           wire.Read(units.time_scale);
}

[[nodiscard]] bool ReadMath(SnapshotWireReader& wire, SnapshotV2Target& target) noexcept
{
    blitzar_core::ExecutionSettings& settings = target.math.settings;

    std::uint8_t raw{};

    if (!wire.Read(raw)) {
        return false;
    }

    settings.mode = static_cast<blitzar_core::ExecutionMode>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    settings.cpu.fma = static_cast<blitzar_core::FmaPolicy>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    settings.cpu.reduction = static_cast<blitzar_core::ReductionPolicy>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    settings.hip.fma = static_cast<blitzar_core::FmaPolicy>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    settings.hip.reduction = static_cast<blitzar_core::ReductionPolicy>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    settings.mpi.fma = static_cast<blitzar_core::FmaPolicy>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    settings.mpi.reduction = static_cast<blitzar_core::ReductionPolicy>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    target.math.compensator = static_cast<SnapshotV2CompensatorKind>(raw);

    return true;
}

[[nodiscard]] bool ReadIdentity(SnapshotWireReader& wire, SnapshotV2Target& target) noexcept
{
    std::uint8_t raw{};
    std::size_t declared{};

    if (!wire.Read(raw)) {
        return false;
    }

    target.backend.backend = static_cast<SnapshotV2BackendKind>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    target.backend.device = static_cast<SnapshotV2DeviceKind>(raw);

    if (!wire.Read(raw)) {
        return false;
    }

    target.backend.precision = static_cast<SnapshotV2PrecisionKind>(raw);

    if (ReadSectionTag(wire, SnapshotV2Section::GlobalDomain, DomainSectionBytes, declared) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    SnapshotV2Domain& domain = target.domain;

    return wire.Read(domain.minimum.x) && wire.Read(domain.minimum.y) &&
           wire.Read(domain.minimum.z) && wire.Read(domain.maximum.x) &&
           wire.Read(domain.maximum.y) && wire.Read(domain.maximum.z) &&
           wire.Read(domain.source_rank_count) && wire.Read(domain.decomposition_version);
}

[[nodiscard]] bool ReadOwnership(
    SnapshotWireReader& wire, SnapshotV2Target& target, std::uint64_t count) noexcept
{
    std::uint8_t kind{};

    if (!wire.Read(kind)) {
        return false;
    }

    target.ownership_kind = static_cast<SnapshotV2OwnershipKind>(kind);

    if (!wire.Read(target.ownership_source_rank_count)) {
        return false;
    }

    std::uint64_t stored_count{};

    if (!wire.Read(stored_count) || stored_count != count) {
        return false;
    }

    return ReadRange(
        wire, std::span<std::uint32_t>(target.source_rank).first(static_cast<std::size_t>(count)));
}

[[nodiscard]] blitzar_status ReadFrameIdentity(SnapshotWireReader& wire) noexcept
{
    std::uint32_t magic{};
    std::uint16_t version{};
    std::uint16_t section_count{};

    if (!wire.ReadUnhashed(magic) || !wire.ReadUnhashed(version) ||
        !wire.ReadUnhashed(section_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (magic != blitzar_core::SnapshotMagic) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (version != SnapshotV2Version || section_count != 8U) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status ValidateParticleSection(SnapshotWireReader& wire,
    std::size_t max_particle_count, std::uint64_t& particle_count) noexcept
{
    std::size_t declared{};
    const blitzar_status status =
        ReadSectionTag(wire, SnapshotV2Section::ParticleState, 0U, declared);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    if (declared < ParticleSectionFixedBytes ||
        (declared - ParticleSectionFixedBytes) % ParticleSectionBytesPerParticle != 0U) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    particle_count = (declared - ParticleSectionFixedBytes) / ParticleSectionBytesPerParticle;

    if (particle_count == 0U || particle_count > max_particle_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return wire.SkipHashed(declared) ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] blitzar_status ValidateFrame(SnapshotWireReader& wire, std::size_t max_particle_count,
    std::uint64_t& particle_count) noexcept
{
    std::size_t declared{};

    const blitzar_status identity_status = ReadFrameIdentity(wire);

    if (identity_status != BLITZAR_STATUS_OK) {
        return identity_status;
    }

    const blitzar_status particle_status =
        ValidateParticleSection(wire, max_particle_count, particle_count);

    if (particle_status != BLITZAR_STATUS_OK) {
        return particle_status;
    }

    const std::array<SectionExpectation, 6> fixed_sections{
        {{SnapshotV2Section::IntegratorState, IntegratorSectionBytes},
            {SnapshotV2Section::RngState, RngSectionBytes},
            {SnapshotV2Section::UnitSystem, UnitSectionBytes},
            {SnapshotV2Section::ExecutionMath, MathSectionBytes},
            {SnapshotV2Section::BackendIdentity, BackendSectionBytes},
            {SnapshotV2Section::GlobalDomain, DomainSectionBytes}}};

    for (const SectionExpectation& section : fixed_sections) {
        const blitzar_status status =
            ReadSectionTag(wire, section.section, section.length, declared);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (!wire.SkipHashed(section.length)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    const std::size_t ownership_length =
        OwnershipSectionFixedBytes + static_cast<std::size_t>(particle_count) * 4U;

    const blitzar_status ownership_status =
        ReadSectionTag(wire, SnapshotV2Section::SourceOwnership, ownership_length, declared);

    if (ownership_status != BLITZAR_STATUS_OK) {
        return ownership_status;
    }

    if (!wire.SkipHashed(ownership_length)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::uint64_t stored_checksum{};

    if (!wire.ReadUnhashed(stored_checksum) || stored_checksum != wire.Checksum() ||
        !wire.AtEnd()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] bool DecodeFrame(
    SnapshotWireReader& wire, SnapshotV2Target& target, std::uint64_t particle_count) noexcept
{
    std::size_t declared{};

    if (ReadFrameIdentity(wire) != BLITZAR_STATUS_OK) {
        return false;
    }

    if (ReadSectionTag(wire, SnapshotV2Section::ParticleState, 0U, declared) != BLITZAR_STATUS_OK) {
        return false;
    }

    if (!ReadParticles(wire, target, particle_count)) {
        return false;
    }

    if (ReadSectionTag(wire, SnapshotV2Section::IntegratorState, IntegratorSectionBytes,
            declared) != BLITZAR_STATUS_OK) {
        return false;
    }

    if (!ReadIntegratorRngUnits(wire, target)) {
        return false;
    }

    if (ReadSectionTag(wire, SnapshotV2Section::ExecutionMath, MathSectionBytes, declared) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    if (!ReadMath(wire, target)) {
        return false;
    }

    if (ReadSectionTag(wire, SnapshotV2Section::BackendIdentity, BackendSectionBytes, declared) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    if (!ReadIdentity(wire, target)) {
        return false;
    }

    const std::size_t ownership_length =
        OwnershipSectionFixedBytes + static_cast<std::size_t>(particle_count) * 4U;

    if (ReadSectionTag(wire, SnapshotV2Section::SourceOwnership, ownership_length, declared) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    if (!ReadOwnership(wire, target, particle_count)) {
        return false;
    }

    return true;
}

} // namespace

SnapshotV2Reader::SnapshotV2Reader(std::size_t max_particle_count)
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount))),
      buffer_(blitzar_core::SnapshotWireMaxBytes)
{
}

blitzar_status SnapshotV2Reader::Read(const std::filesystem::path& path, SnapshotV2Target& target)
{
    try {
        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        input.read(
            reinterpret_cast<char*>(buffer_.data()), static_cast<std::streamsize>(buffer_.size()));

        const std::size_t bytes = static_cast<std::size_t>(input.gcount());
        const bool oversized = !input.eof() && !input.fail();

        if (input.bad() || oversized || bytes < SnapshotV2HeaderBytes) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::span<const std::byte> frame(buffer_.data(), bytes);
        SnapshotWireReader validator(frame);
        std::uint64_t particle_count{};

        const blitzar_status validated =
            ValidateFrame(validator, max_particle_count_, particle_count);

        if (validated != BLITZAR_STATUS_OK) {
            return validated;
        }

        if (!target.HasCapacity(particle_count)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        SnapshotWireReader decoder(frame);

        const bool decoded = DecodeFrame(decoder, target, particle_count);
        const bool valid = decoded && target.IsValid(particle_count);

        if (!valid) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        return BLITZAR_STATUS_OK;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::ios_base::failure&) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace blitzar_io
