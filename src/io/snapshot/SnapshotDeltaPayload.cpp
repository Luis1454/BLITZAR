#include "io/snapshot/SnapshotDeltaPayload.hpp"

#include "io/snapshot/SnapshotChecksum.hpp"
#include "io/snapshot/SnapshotWire.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace blitzar_io {

namespace {

inline constexpr std::size_t PayloadBytesPerParticle =
    static_cast<std::size_t>(blitzar_core::SnapshotWireBytesPerParticle);

template <SnapshotWireUnsigned Unsigned>
[[nodiscard]] bool AppendInteger(
    Unsigned value, std::span<std::byte> destination, std::size_t& position) noexcept
{
    const auto bytes = EncodeLittleEndian(value);

    if (position > destination.size() || bytes.size() > destination.size() - position) {
        return false;
    }

    std::copy(bytes.begin(), bytes.end(), destination.begin() + position);

    position += bytes.size();

    return true;
}

[[nodiscard]] bool AppendValue(
    std::uint64_t value, std::span<std::byte> destination, std::size_t& position) noexcept
{
    return AppendInteger(value, destination, position);
}

[[nodiscard]] bool AppendValue(
    blitzar_core::Scalar value, std::span<std::byte> destination, std::size_t& position) noexcept
{
    return AppendInteger(std::bit_cast<std::uint64_t>(value), destination, position);
}

template <typename Value>
[[nodiscard]] bool PackRange(
    std::span<const Value> values, std::span<std::byte> destination, std::size_t& position) noexcept
{
    for (const Value value : values) {
        if (!AppendValue(value, destination, position)) {
            return false;
        }
    }

    return true;
}

template <SnapshotWireUnsigned Unsigned>
[[nodiscard]] bool ReadInteger(
    std::span<const std::byte> source, std::size_t& position, Unsigned& value) noexcept
{
    if (position > source.size() || sizeof(Unsigned) > source.size() - position) {
        return false;
    }

    value = DecodeLittleEndian<Unsigned>(source.subspan(position, sizeof(Unsigned)));
    position += sizeof(Unsigned);

    return true;
}

[[nodiscard]] bool ReadValue(
    std::span<const std::byte> source, std::size_t& position, std::uint64_t& value) noexcept
{
    return ReadInteger(source, position, value);
}

[[nodiscard]] bool ReadValue(
    std::span<const std::byte> source, std::size_t& position, blitzar_core::Scalar& value) noexcept
{
    std::uint64_t bits{};

    if (!ReadInteger(source, position, bits)) {
        return false;
    }

    value = std::bit_cast<blitzar_core::Scalar>(bits);

    return true;
}

template <typename Value>
[[nodiscard]] bool UnpackRange(
    std::span<const std::byte> source, std::size_t& position, std::span<Value> values) noexcept
{
    for (Value& value : values) {
        if (!ReadValue(source, position, value)) {
            return false;
        }
    }

    return true;
}

} // namespace

bool SnapshotDeltaPayload::Pack(
    blitzar_core::SnapshotPayloadView payload, std::span<std::byte> destination) noexcept
{
    std::size_t position{};

    return PackRange(payload.ids, destination, position) &&
           PackRange(payload.position_x, destination, position) &&
           PackRange(payload.position_y, destination, position) &&
           PackRange(payload.position_z, destination, position) &&
           PackRange(payload.velocity_x, destination, position) &&
           PackRange(payload.velocity_y, destination, position) &&
           PackRange(payload.velocity_z, destination, position) &&
           PackRange(payload.mass, destination, position) && position == destination.size();
}

bool SnapshotDeltaPayload::Unpack(std::span<const std::byte> source,
    blitzar_core::SnapshotMutablePayloadView destination) noexcept
{
    if (source.size() % PayloadBytesPerParticle != 0) {
        return false;
    }

    const std::size_t count = source.size() / PayloadBytesPerParticle;

    if (!destination.HasCapacity(static_cast<std::uint64_t>(count))) {
        return false;
    }

    std::size_t position{};

    return UnpackRange(source, position, destination.ids.first(count)) &&
           UnpackRange(source, position, destination.position_x.first(count)) &&
           UnpackRange(source, position, destination.position_y.first(count)) &&
           UnpackRange(source, position, destination.position_z.first(count)) &&
           UnpackRange(source, position, destination.velocity_x.first(count)) &&
           UnpackRange(source, position, destination.velocity_y.first(count)) &&
           UnpackRange(source, position, destination.velocity_z.first(count)) &&
           UnpackRange(source, position, destination.mass.first(count)) &&
           position == source.size();
}

std::uint64_t SnapshotDeltaPayload::Checksum(std::span<const std::byte> bytes) noexcept
{
    SnapshotChecksum checksum;

    checksum.Add(bytes);

    return checksum.Value();
}

} // namespace blitzar_io
