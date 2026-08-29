#include "io/snapshot/SnapshotDeltaStream.hpp"

#include "io/snapshot/SnapshotWire.hpp"

#include <algorithm>
#include <cstddef>

namespace blitzar_io {

namespace {

inline constexpr std::size_t DeltaHeaderReserved = 0;
inline constexpr unsigned char ZeroRunFlag = 0x80U;

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

} // namespace

bool SnapshotDeltaStream::WriteHeader(std::span<std::byte> destination, std::size_t& position,
    SnapshotDeltaStreamHeader header) noexcept
{
    return AppendInteger(SnapshotDeltaMagic, destination, position) &&
           AppendInteger(SnapshotDeltaVersion, destination, position) &&
           AppendInteger(static_cast<std::uint16_t>(DeltaHeaderReserved), destination, position) &&
           AppendInteger(header.raw_bytes, destination, position) &&
           AppendInteger(header.checksum, destination, position);
}

bool SnapshotDeltaStream::ReadHeader(std::span<const std::byte> source, std::size_t& position,
    SnapshotDeltaStreamHeader& header) noexcept
{
    std::uint32_t magic{};
    std::uint16_t version{};
    std::uint16_t reserved{};

    if (!ReadInteger(source, position, magic) || !ReadInteger(source, position, version) ||
        !ReadInteger(source, position, reserved) ||
        !ReadInteger(source, position, header.raw_bytes) ||
        !ReadInteger(source, position, header.checksum)) {
        return false;
    }

    return magic == SnapshotDeltaMagic && version == SnapshotDeltaVersion && reserved == 0;
}

bool SnapshotDeltaStream::EncodeRuns(std::span<const std::byte> base,
    std::span<const std::byte> current, std::span<std::byte> destination,
    std::size_t& position) noexcept
{
    if (base.size() != current.size()) {
        return false;
    }

    std::size_t index{};

    while (index < current.size()) {
        const bool zero_run = (current[index] ^ base[index]) == std::byte{};
        std::size_t run_length = 1;

        while (
            run_length < SnapshotDeltaMaxRunLength && index + run_length < current.size() &&
            ((current[index + run_length] ^ base[index + run_length]) == std::byte{}) == zero_run) {
            ++run_length;
        }

        if (position >= destination.size()) {
            return false;
        }

        const unsigned char control = static_cast<unsigned char>(
            (zero_run ? ZeroRunFlag : 0U) | static_cast<unsigned char>(run_length - 1));

        destination[position++] = std::byte{control};

        if (!zero_run) {
            if (run_length > destination.size() - position) {
                return false;
            }

            for (std::size_t offset{}; offset < run_length; ++offset) {
                destination[position + offset] = current[index + offset] ^ base[index + offset];
            }

            position += run_length;
        }

        index += run_length;
    }

    return true;
}

bool SnapshotDeltaStream::DecodeRuns(std::span<const std::byte> base,
    std::span<const std::byte> source, std::span<std::byte> decoded, std::size_t& position) noexcept
{
    if (base.size() != decoded.size()) {
        return false;
    }

    std::size_t output{};

    while (output < decoded.size()) {
        if (position >= source.size()) {
            return false;
        }

        const unsigned char control = std::to_integer<unsigned char>(source[position++]);

        const bool zero_run = (control & ZeroRunFlag) != 0;
        const std::size_t run_length = static_cast<std::size_t>(control & (ZeroRunFlag - 1U)) + 1;

        if (run_length > decoded.size() - output) {
            return false;
        }

        if (zero_run) {
            std::copy_n(base.begin() + output, run_length, decoded.begin() + output);
        }
        else {
            if (run_length > source.size() - position) {
                return false;
            }

            for (std::size_t offset{}; offset < run_length; ++offset) {
                decoded[output + offset] = base[output + offset] ^ source[position + offset];
            }

            position += run_length;
        }

        output += run_length;
    }

    return true;
}

} // namespace blitzar_io
