#include "io/snapshot/SnapshotChecksum.hpp"

#include <array>
#include <bit>

namespace blitzar_io {

namespace {

inline constexpr std::uint64_t FnvOffsetBasis = 14695981039346656037ULL;
inline constexpr std::uint64_t FnvPrime = 1099511628211ULL;

[[nodiscard]] std::array<std::byte, sizeof(std::uint64_t)> EncodeLittleEndian(
    std::uint64_t value) noexcept
{
    std::array<std::byte, sizeof(std::uint64_t)> bytes{};

    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = std::byte{static_cast<unsigned char>(value >> (index * 8U))};
    }

    return bytes;
}

} // namespace

SnapshotChecksum::SnapshotChecksum() noexcept : value_(FnvOffsetBasis) {}

void SnapshotChecksum::Add(std::span<const std::byte> bytes) noexcept
{
    for (const std::byte byte : bytes) {
        value_ = (value_ ^ std::to_integer<unsigned char>(byte)) * FnvPrime;
    }
}

void SnapshotChecksum::Add(std::uint64_t value) noexcept
{
    const auto bytes = EncodeLittleEndian(value);

    Add(std::span<const std::byte>(bytes));
}

void SnapshotChecksum::Add(blitzar_core::Scalar value) noexcept
{
    Add(std::bit_cast<std::uint64_t>(value));
}

std::uint64_t SnapshotChecksum::Value() const noexcept
{
    return value_;
}

} // namespace blitzar_io
