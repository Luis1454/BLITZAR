#include "io/snapshot/SnapshotReader.hpp"

#include "io/snapshot/SnapshotWire.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <ios>
#include <limits>
#include <new>
#include <span>
#include <system_error>

namespace blitzar_io {

namespace {

[[nodiscard]] std::size_t MaximumWireBytes(std::size_t max_particle_count) noexcept
{
    const std::uint64_t count = static_cast<std::uint64_t>(max_particle_count);
    const std::uint64_t bytes = blitzar_core::SnapshotWireHeaderBytes +
                                count * blitzar_core::SnapshotWireBytesPerParticle +
                                blitzar_core::SnapshotWireChecksumBytes;

    return static_cast<std::size_t>(bytes);
}

[[nodiscard]] bool WireSize(std::uint64_t particle_count, std::size_t& size) noexcept
{
    if (particle_count > blitzar_core::SnapshotMaxParticleCount) {
        return false;
    }

    const std::uint64_t bytes = blitzar_core::SnapshotWireHeaderBytes +
                                particle_count * blitzar_core::SnapshotWireBytesPerParticle +
                                blitzar_core::SnapshotWireChecksumBytes;

    if (bytes > std::numeric_limits<std::size_t>::max()) {
        return false;
    }

    size = static_cast<std::size_t>(bytes);

    return true;
}

[[nodiscard]] bool ReadHeader(
    SnapshotWireReader& wire, blitzar_core::SnapshotHeader& header) noexcept
{
    std::uint16_t version{};
    std::uint8_t endianness{};
    std::uint8_t distribution{};
    std::uint8_t id_policy{};

    if (!wire.ReadUnhashed(header.magic) || !wire.ReadUnhashed(version) ||
        !wire.ReadUnhashed(header.scalar_bytes) || !wire.ReadUnhashed(header.particle_count) ||
        !wire.ReadUnhashed(header.step) || !wire.ReadUnhashed(header.time) ||
        !wire.ReadUnhashed(header.rank_count) || !wire.ReadUnhashed(header.rank_index) ||
        !wire.ReadUnhashed(endianness) || !wire.ReadUnhashed(distribution) ||
        !wire.ReadUnhashed(id_policy)) {
        return false;
    }

    header.version = static_cast<blitzar_core::SnapshotVersion>(version);
    header.endianness = static_cast<blitzar_core::SnapshotEndianness>(endianness);
    header.distribution = static_cast<blitzar_core::SnapshotDistribution>(distribution);
    header.id_policy = static_cast<blitzar_core::SnapshotIdPolicy>(id_policy);

    return true;
}

[[nodiscard]] bool ValidateIds(SnapshotWireReader& wire, std::size_t count) noexcept
{
    for (std::size_t index = 0; index < count; ++index) {
        std::uint64_t id{};

        if (!wire.Read(id) || id != index) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool ValidateScalars(
    SnapshotWireReader& wire, std::size_t count, bool require_non_negative) noexcept
{
    for (std::size_t index = 0; index < count; ++index) {
        blitzar_core::Scalar value{};

        if (!wire.Read(value) || !std::isfinite(value) || (require_non_negative && value < 0.0)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool ValidatePayload(SnapshotWireReader& wire, std::size_t count) noexcept
{
    return ValidateIds(wire, count) && ValidateScalars(wire, count, false) &&
           ValidateScalars(wire, count, false) && ValidateScalars(wire, count, false) &&
           ValidateScalars(wire, count, false) && ValidateScalars(wire, count, false) &&
           ValidateScalars(wire, count, false) && ValidateScalars(wire, count, true);
}

void DecodeIds(SnapshotWireReader& wire, std::size_t count, std::span<std::uint64_t> ids) noexcept
{
    for (std::size_t index = 0; index < count; ++index) {
        std::uint64_t id{};

        (void)wire.Read(id);

        ids[index] = id;
    }
}

void DecodeScalars(
    SnapshotWireReader& wire, std::size_t count, std::span<blitzar_core::Scalar> values) noexcept
{
    for (std::size_t index = 0; index < count; ++index) {
        blitzar_core::Scalar value{};

        (void)wire.Read(value);

        values[index] = value;
    }
}

void DecodePayload(SnapshotWireReader& wire, std::size_t count,
    blitzar_core::SnapshotMutablePayloadView payload) noexcept
{
    DecodeIds(wire, count, payload.ids);
    DecodeScalars(wire, count, payload.position_x);
    DecodeScalars(wire, count, payload.position_y);
    DecodeScalars(wire, count, payload.position_z);
    DecodeScalars(wire, count, payload.velocity_x);
    DecodeScalars(wire, count, payload.velocity_y);
    DecodeScalars(wire, count, payload.velocity_z);
    DecodeScalars(wire, count, payload.mass);
}

} // namespace

SnapshotReader::SnapshotReader(std::size_t max_particle_count)
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount))),
      buffer_(MaximumWireBytes(max_particle_count_))
{
}

blitzar_status SnapshotReader::Read(const std::filesystem::path& path,
    blitzar_core::SnapshotHeader& header, blitzar_core::SnapshotMutablePayloadView payload)
{
    if (path.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        std::error_code file_error;
        const std::uintmax_t file_size = std::filesystem::file_size(path, file_error);

        if (file_error) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        if (file_size > buffer_.size() || file_size < blitzar_core::SnapshotWireHeaderBytes +
                                                          blitzar_core::SnapshotWireChecksumBytes) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t byte_count = static_cast<std::size_t>(file_size);
        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        input.read(
            reinterpret_cast<char*>(buffer_.data()), static_cast<std::streamsize>(byte_count));

        if (input.gcount() != static_cast<std::streamsize>(byte_count)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::span<const std::byte> bytes =
            std::span<const std::byte>(buffer_).first(byte_count);

        SnapshotWireReader wire(bytes);
        blitzar_core::SnapshotHeader candidate{};

        if (!ReadHeader(wire, candidate)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_status header_status = candidate.Validate();

        if (header_status != BLITZAR_STATUS_OK) {
            return header_status;
        }

        std::size_t expected_size{};

        if (!WireSize(candidate.particle_count, expected_size) || byte_count != expected_size ||
            !payload.HasCapacity(candidate.particle_count)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t count = static_cast<std::size_t>(candidate.particle_count);

        if (!ValidatePayload(wire, count)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::uint64_t expected_checksum{};

        if (!wire.ReadUnhashed(expected_checksum) || !wire.AtEnd() ||
            expected_checksum != wire.Checksum()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        SnapshotWireReader decode_wire(bytes);
        blitzar_core::SnapshotHeader ignored_header{};

        (void)ReadHeader(decode_wire, ignored_header);

        DecodePayload(decode_wire, count, payload);

        header = candidate;

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
