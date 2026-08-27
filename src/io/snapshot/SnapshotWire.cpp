#include "io/snapshot/SnapshotWire.hpp"

#include <algorithm>
#include <bit>
#include <ostream>

namespace blitzar_io {

namespace {

inline constexpr std::uint64_t FnvOffsetBasis = 14695981039346656037ULL;
inline constexpr std::uint64_t FnvPrime = 1099511628211ULL;

} // namespace

SnapshotWireWriter::SnapshotWireWriter(std::ostream& output) noexcept
    : output_(output), checksum_(FnvOffsetBasis)
{
}

bool SnapshotWireWriter::Put(blitzar_core::Scalar value) noexcept
{
    return Put(std::bit_cast<std::uint64_t>(value));
}

bool SnapshotWireWriter::PutUnhashed(blitzar_core::Scalar value) noexcept
{
    return PutUnhashed(std::bit_cast<std::uint64_t>(value));
}

std::uint64_t SnapshotWireWriter::Checksum() const noexcept
{
    return checksum_;
}

bool SnapshotWireWriter::PutBytes(std::span<const std::byte> bytes, bool hash) noexcept
{
    output_.write(
        reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    if (!output_) {
        return false;
    }

    if (hash) {
        for (const std::byte byte : bytes) {
            checksum_ = (checksum_ ^ std::to_integer<unsigned char>(byte)) * FnvPrime;
        }
    }

    return true;
}

SnapshotWireReader::SnapshotWireReader(std::span<const std::byte> bytes) noexcept
    : bytes_(bytes), position_(0), checksum_(FnvOffsetBasis)
{
}

bool SnapshotWireReader::Read(blitzar_core::Scalar& value) noexcept
{
    std::uint64_t bits{};

    if (!Read(bits)) {
        return false;
    }

    value = std::bit_cast<blitzar_core::Scalar>(bits);

    return true;
}

bool SnapshotWireReader::ReadUnhashed(blitzar_core::Scalar& value) noexcept
{
    std::uint64_t bits{};

    if (!ReadUnhashed(bits)) {
        return false;
    }

    value = std::bit_cast<blitzar_core::Scalar>(bits);

    return true;
}

bool SnapshotWireReader::AtEnd() const noexcept
{
    return position_ == bytes_.size();
}

std::uint64_t SnapshotWireReader::Checksum() const noexcept
{
    return checksum_;
}

bool SnapshotWireReader::ReadBytes(std::span<std::byte> destination, bool hash) noexcept
{
    if (position_ > bytes_.size() || destination.size() > bytes_.size() - position_) {
        return false;
    }

    const std::span<const std::byte> source = bytes_.subspan(position_, destination.size());

    std::copy(source.begin(), source.end(), destination.begin());

    position_ += destination.size();

    if (hash) {
        for (const std::byte byte : source) {
            checksum_ = (checksum_ ^ std::to_integer<unsigned char>(byte)) * FnvPrime;
        }
    }

    return true;
}

} // namespace blitzar_io
