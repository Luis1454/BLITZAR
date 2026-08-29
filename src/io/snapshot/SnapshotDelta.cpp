#include "io/snapshot/SnapshotDelta.hpp"

#include "io/snapshot/SnapshotDeltaPayload.hpp"
#include "io/snapshot/SnapshotDeltaStream.hpp"

#include <algorithm>
#include <limits>
#include <span>

namespace blitzar_io {

namespace {

inline constexpr std::size_t PayloadBytesPerParticle =
    static_cast<std::size_t>(blitzar_core::SnapshotWireBytesPerParticle);

[[nodiscard]] bool HasPayloadShape(blitzar_core::SnapshotPayloadView payload,
    std::size_t expected_count, std::size_t max_particle_count) noexcept
{
    if (expected_count > max_particle_count ||
        expected_count > std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }

    return payload.HasMatchingCounts(static_cast<std::uint64_t>(expected_count));
}

} // namespace

SnapshotDelta::SnapshotDelta(std::size_t max_particle_count) noexcept
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount)))
{
}

std::size_t SnapshotDelta::RawPayloadBytes(std::size_t particle_count) const noexcept
{
    if (particle_count > max_particle_count_ ||
        particle_count > std::numeric_limits<std::size_t>::max() / PayloadBytesPerParticle) {
        return 0;
    }

    return particle_count * PayloadBytesPerParticle;
}

std::size_t SnapshotDelta::MaxEncodedBytes(std::size_t particle_count) const noexcept
{
    if (particle_count > max_particle_count_) {
        return 0;
    }

    const std::size_t raw_bytes = RawPayloadBytes(particle_count);

    if (raw_bytes > (std::numeric_limits<std::size_t>::max() - SnapshotDeltaHeaderBytes) / 2) {
        return 0;
    }

    return SnapshotDeltaHeaderBytes + raw_bytes * 2;
}

std::size_t SnapshotDelta::WorkspaceBytes(std::size_t particle_count) const noexcept
{
    const std::size_t raw_bytes = RawPayloadBytes(particle_count);

    if (raw_bytes > std::numeric_limits<std::size_t>::max() / 3) {
        return 0;
    }

    return raw_bytes * 3;
}

blitzar_status SnapshotDelta::Encode(blitzar_core::SnapshotPayloadView base,
    blitzar_core::SnapshotPayloadView current, SnapshotDeltaBuffer& buffers) const noexcept
{
    buffers.encoded_size = 0;

    const std::size_t count = base.ids.size();

    if (!HasPayloadShape(base, count, max_particle_count_) ||
        !HasPayloadShape(current, count, max_particle_count_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t raw_bytes = RawPayloadBytes(count);

    if (buffers.base.size() < raw_bytes || buffers.current.size() < raw_bytes ||
        buffers.encoded.size() < SnapshotDeltaHeaderBytes) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::span<std::byte> base_bytes = buffers.base.first(raw_bytes);
    const std::span<std::byte> current_bytes = buffers.current.first(raw_bytes);

    if (!SnapshotDeltaPayload::Pack(base, base_bytes) ||
        !SnapshotDeltaPayload::Pack(current, current_bytes)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const SnapshotDeltaStreamHeader header{
        static_cast<std::uint64_t>(raw_bytes), SnapshotDeltaPayload::Checksum(current_bytes)};

    std::size_t position{};

    if (!SnapshotDeltaStream::WriteHeader(buffers.encoded, position, header) ||
        !SnapshotDeltaStream::EncodeRuns(base_bytes, current_bytes, buffers.encoded, position)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    buffers.encoded_size = position;

    return BLITZAR_STATUS_OK;
}

blitzar_status SnapshotDelta::Decode(blitzar_core::SnapshotPayloadView base,
    SnapshotDeltaBuffer& buffers,
    blitzar_core::SnapshotMutablePayloadView destination) const noexcept
{
    if (buffers.encoded_size > buffers.encoded.size() ||
        buffers.encoded_size < SnapshotDeltaHeaderBytes) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t count = base.ids.size();

    if (!HasPayloadShape(base, count, max_particle_count_) ||
        !destination.HasCapacity(static_cast<std::uint64_t>(count))) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t raw_bytes = RawPayloadBytes(count);

    if (buffers.base.size() < raw_bytes || buffers.decoded.size() < raw_bytes ||
        raw_bytes > std::numeric_limits<std::uint64_t>::max()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::span<const std::byte> encoded = buffers.encoded.first(buffers.encoded_size);
    const std::span<std::byte> base_bytes = buffers.base.first(raw_bytes);
    const std::span<std::byte> decoded_bytes = buffers.decoded.first(raw_bytes);
    std::size_t position{};
    SnapshotDeltaStreamHeader header{};

    if (!SnapshotDeltaStream::ReadHeader(encoded, position, header) ||
        header.raw_bytes != static_cast<std::uint64_t>(raw_bytes) ||
        !SnapshotDeltaPayload::Pack(base, base_bytes) ||
        !SnapshotDeltaStream::DecodeRuns(base_bytes, encoded, decoded_bytes, position) ||
        position != encoded.size() ||
        SnapshotDeltaPayload::Checksum(decoded_bytes) != header.checksum) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return SnapshotDeltaPayload::Unpack(decoded_bytes, destination)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace blitzar_io
