#include "io/snapshot/SnapshotWriter.hpp"

#include "io/snapshot/SnapshotWire.hpp"

#include <algorithm>
#include <fstream>
#include <ios>
#include <new>
#include <span>

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

[[nodiscard]] bool WriteHeader(
    SnapshotWireWriter& wire, const blitzar_core::SnapshotHeader& header) noexcept
{
    return wire.PutUnhashed(header.magic) &&
           wire.PutUnhashed(static_cast<std::uint16_t>(header.version)) &&
           wire.PutUnhashed(header.scalar_bytes) && wire.PutUnhashed(header.particle_count) &&
           wire.PutUnhashed(header.step) && wire.PutUnhashed(header.time) &&
           wire.PutUnhashed(header.rank_count) && wire.PutUnhashed(header.rank_index) &&
           wire.PutUnhashed(static_cast<std::uint8_t>(header.endianness)) &&
           wire.PutUnhashed(static_cast<std::uint8_t>(header.distribution)) &&
           wire.PutUnhashed(static_cast<std::uint8_t>(header.id_policy));
}

[[nodiscard]] bool WritePayload(
    SnapshotWireWriter& wire, const blitzar_core::SnapshotPayloadView& payload) noexcept
{
    return WriteRange(wire, payload.ids) && WriteRange(wire, payload.position_x) &&
           WriteRange(wire, payload.position_y) && WriteRange(wire, payload.position_z) &&
           WriteRange(wire, payload.velocity_x) && WriteRange(wire, payload.velocity_y) &&
           WriteRange(wire, payload.velocity_z) && WriteRange(wire, payload.mass);
}

} // namespace

SnapshotWriter::SnapshotWriter(std::size_t max_particle_count) noexcept
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount)))
{
}

blitzar_status SnapshotWriter::Write(
    const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame) const
{
    if (path.empty() || frame.header.particle_count > max_particle_count_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status frame_status = frame.Validate();

    if (frame_status != BLITZAR_STATUS_OK) {
        return frame_status;
    }

    try {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);

        if (!output.is_open()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        SnapshotWireWriter wire(output);

        if (!WriteHeader(wire, frame.header) || !WritePayload(wire, frame.payload) ||
            !wire.PutUnhashed(wire.Checksum())) {
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

} // namespace blitzar_io
