#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_WIRE_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_WIRE_HPP

#include "core/CoreSnapshot.hpp"
#include "io/snapshot/SnapshotChecksum.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <span>

namespace blitzar_io {

template <typename Value>
concept SnapshotWireUnsigned =
    std::same_as<Value, std::uint8_t> || std::same_as<Value, std::uint16_t> ||
    std::same_as<Value, std::uint32_t> || std::same_as<Value, std::uint64_t>;

template <SnapshotWireUnsigned Unsigned>
[[nodiscard]] inline std::array<std::byte, sizeof(Unsigned)> EncodeLittleEndian(
    Unsigned value) noexcept
{
    std::array<std::byte, sizeof(Unsigned)> bytes{};

    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = std::byte{static_cast<unsigned char>(value >> (index * 8U))};
    }

    return bytes;
}

template <SnapshotWireUnsigned Unsigned>
[[nodiscard]] inline Unsigned DecodeLittleEndian(std::span<const std::byte> bytes) noexcept
{
    Unsigned value{};

    for (std::size_t index = 0; index < bytes.size(); ++index) {
        value |= static_cast<Unsigned>(std::to_integer<unsigned char>(bytes[index]))
                 << (index * 8U);
    }

    return value;
}

class SnapshotWireWriter final {
public:
    explicit SnapshotWireWriter(std::ostream& output) noexcept;

    template <SnapshotWireUnsigned Unsigned> [[nodiscard]] bool Put(Unsigned value) noexcept
    {
        const auto bytes = EncodeLittleEndian(value);

        return PutBytes(bytes, true);
    }

    [[nodiscard]] bool Put(blitzar_core::Scalar value) noexcept;

    template <SnapshotWireUnsigned Unsigned> [[nodiscard]] bool PutUnhashed(Unsigned value) noexcept
    {
        const auto bytes = EncodeLittleEndian(value);

        return PutBytes(bytes, false);
    }

    [[nodiscard]] bool PutUnhashed(blitzar_core::Scalar value) noexcept;

    [[nodiscard]] std::uint64_t Checksum() const noexcept;

private:
    [[nodiscard]] bool PutBytes(std::span<const std::byte> bytes, bool hash) noexcept;

    std::ostream& output_;
    SnapshotChecksum checksum_;
};

class SnapshotWireReader final {
public:
    explicit SnapshotWireReader(std::span<const std::byte> bytes) noexcept;

    template <SnapshotWireUnsigned Unsigned> [[nodiscard]] bool Read(Unsigned& value) noexcept
    {
        std::array<std::byte, sizeof(Unsigned)> bytes{};

        if (!ReadBytes(bytes, true)) {
            return false;
        }

        value = DecodeLittleEndian<Unsigned>(bytes);

        return true;
    }

    [[nodiscard]] bool Read(blitzar_core::Scalar& value) noexcept;

    template <SnapshotWireUnsigned Unsigned>
    [[nodiscard]] bool ReadUnhashed(Unsigned& value) noexcept
    {
        std::array<std::byte, sizeof(Unsigned)> bytes{};

        if (!ReadBytes(bytes, false)) {
            return false;
        }

        value = DecodeLittleEndian<Unsigned>(bytes);

        return true;
    }

    [[nodiscard]] bool ReadUnhashed(blitzar_core::Scalar& value) noexcept;

    [[nodiscard]] bool AtEnd() const noexcept;
    [[nodiscard]] std::uint64_t Checksum() const noexcept;

private:
    [[nodiscard]] bool ReadBytes(std::span<std::byte> destination, bool hash) noexcept;

    std::span<const std::byte> bytes_;
    std::size_t position_;
    SnapshotChecksum checksum_;
};

} // namespace blitzar_io

#endif
