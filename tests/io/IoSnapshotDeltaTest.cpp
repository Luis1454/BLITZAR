#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/snapshot/SnapshotChecksum.hpp"
#include "io/snapshot/SnapshotDelta.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>

namespace {

inline constexpr std::size_t ParticleCount = 32;
inline constexpr std::size_t RepeatCount = 32;

[[nodiscard]] constexpr std::size_t RawPayloadSize() noexcept
{
    return ParticleCount * static_cast<std::size_t>(blitzar_core::SnapshotWireBytesPerParticle);
}

inline constexpr std::size_t RawBytes = RawPayloadSize();
inline constexpr std::size_t EncodedBytes = blitzar_io::SnapshotDeltaHeaderBytes + 2 * RawBytes;
inline constexpr std::size_t KeyframeInterval = 8;

struct SnapshotStorage final {
    std::array<std::uint64_t, ParticleCount> ids{};
    std::array<blitzar_core::Scalar, ParticleCount> position_x{};
    std::array<blitzar_core::Scalar, ParticleCount> position_y{};
    std::array<blitzar_core::Scalar, ParticleCount> position_z{};
    std::array<blitzar_core::Scalar, ParticleCount> velocity_x{};
    std::array<blitzar_core::Scalar, ParticleCount> velocity_y{};
    std::array<blitzar_core::Scalar, ParticleCount> velocity_z{};
    std::array<blitzar_core::Scalar, ParticleCount> mass{};
};

struct ReplayCase final {
    SnapshotStorage base{};
    SnapshotStorage current{};
    SnapshotStorage next{};
};

struct TimingEvidence final {
    std::uint64_t reference_write_ns{};
    std::uint64_t reference_read_ns{};
    std::uint64_t delta_write_ns{};
    std::uint64_t delta_read_ns{};
};

void FillStorage(SnapshotStorage& storage, std::uint64_t id_base, double offset) noexcept
{
    for (std::size_t index{}; index < ParticleCount; ++index) {
        const double value = static_cast<double>(index);

        storage.ids[index] = id_base + index;
        storage.position_x[index] = 1.0 + value * 0.125 + offset;
        storage.position_y[index] = -2.0 + value * 0.25 + offset;
        storage.position_z[index] = 3.0 + value * 0.5 + offset;
        storage.velocity_x[index] = 0.01 + value * 0.001 + offset;
        storage.velocity_y[index] = -0.02 + value * 0.002 + offset;
        storage.velocity_z[index] = 0.03 + value * 0.003 + offset;
        storage.mass[index] = 1.0 + static_cast<double>(index % 5) * 0.25;
    }
}

[[nodiscard]] blitzar_core::SnapshotPayloadView MakeView(const SnapshotStorage& storage) noexcept
{
    return {std::span<const std::uint64_t>(storage.ids),
        std::span<const blitzar_core::Scalar>(storage.position_x),
        std::span<const blitzar_core::Scalar>(storage.position_y),
        std::span<const blitzar_core::Scalar>(storage.position_z),
        std::span<const blitzar_core::Scalar>(storage.velocity_x),
        std::span<const blitzar_core::Scalar>(storage.velocity_y),
        std::span<const blitzar_core::Scalar>(storage.velocity_z),
        std::span<const blitzar_core::Scalar>(storage.mass)};
}

[[nodiscard]] blitzar_core::SnapshotMutablePayloadView MakeTarget(SnapshotStorage& storage) noexcept
{
    return {std::span<std::uint64_t>(storage.ids),
        std::span<blitzar_core::Scalar>(storage.position_x),
        std::span<blitzar_core::Scalar>(storage.position_y),
        std::span<blitzar_core::Scalar>(storage.position_z),
        std::span<blitzar_core::Scalar>(storage.velocity_x),
        std::span<blitzar_core::Scalar>(storage.velocity_y),
        std::span<blitzar_core::Scalar>(storage.velocity_z),
        std::span<blitzar_core::Scalar>(storage.mass)};
}

[[nodiscard]] bool SameState(const SnapshotStorage& first, const SnapshotStorage& second) noexcept
{
    return first.ids == second.ids && first.position_x == second.position_x &&
           first.position_y == second.position_y && first.position_z == second.position_z &&
           first.velocity_x == second.velocity_x && first.velocity_y == second.velocity_y &&
           first.velocity_z == second.velocity_z && first.mass == second.mass;
}

[[nodiscard]] std::uint64_t Checksum(std::span<const std::byte> bytes) noexcept
{
    blitzar_io::SnapshotChecksum checksum;

    checksum.Add(bytes);

    return checksum.Value();
}

template <typename Operation>
[[nodiscard]] std::uint64_t Measure(Operation operation, std::size_t repetitions) noexcept
{
    volatile std::uint64_t observed{};
    const auto start = std::chrono::steady_clock::now();

    for (std::size_t index{}; index < repetitions; ++index) {
        observed = observed ^ operation();
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start);

    const std::uint64_t average = static_cast<std::uint64_t>(elapsed.count()) / repetitions;

    return std::max<std::uint64_t>(
        1, average + (observed == std::numeric_limits<std::uint64_t>::max()));
}

int CheckRoundTrip(const blitzar_io::SnapshotDelta& codec, const ReplayCase& replay,
    blitzar_io::SnapshotDeltaBuffer& buffers) noexcept
{
    SnapshotStorage decoded{};

    BLITZAR_CHECK(codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(buffers.encoded_size < RawBytes);
    BLITZAR_CHECK(
        codec.Decode(MakeView(replay.base), buffers, MakeTarget(decoded)) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(SameState(replay.current, decoded));

    return 0;
}

int CheckDeterminism(const blitzar_io::SnapshotDelta& codec, const ReplayCase& replay,
    blitzar_io::SnapshotDeltaBuffer& buffers) noexcept
{
    std::array<std::byte, EncodedBytes> first{};

    BLITZAR_CHECK(codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers) ==
                  BLITZAR_STATUS_OK);

    const std::size_t first_size = buffers.encoded_size;

    std::copy_n(buffers.encoded.begin(), first_size, first.begin());

    BLITZAR_CHECK(codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(buffers.encoded_size == first_size);
    BLITZAR_CHECK(std::equal(first.begin(), first.begin() + first_size, buffers.encoded.begin()));

    return 0;
}

int CheckCorruption(const blitzar_io::SnapshotDelta& codec, const ReplayCase& replay,
    blitzar_io::SnapshotDeltaBuffer& buffers) noexcept
{
    SnapshotStorage target{};

    FillStorage(target, 7000, 0.5);

    const SnapshotStorage unchanged = target;

    BLITZAR_CHECK(codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers) ==
                  BLITZAR_STATUS_OK);

    buffers.encoded[16] ^= std::byte{1};

    BLITZAR_CHECK(codec.Decode(MakeView(replay.base), buffers, MakeTarget(target)) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(SameState(target, unchanged));

    buffers.encoded[16] ^= std::byte{1};

    const std::size_t valid_size = buffers.encoded_size;

    buffers.encoded_size = valid_size - 1;

    BLITZAR_CHECK(codec.Decode(MakeView(replay.base), buffers, MakeTarget(target)) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(SameState(target, unchanged));

    buffers.encoded_size = valid_size;

    return 0;
}

int CheckReplay(const blitzar_io::SnapshotDelta& codec, const ReplayCase& replay,
    blitzar_io::SnapshotDeltaBuffer& buffers) noexcept
{
    SnapshotStorage after_current{};
    SnapshotStorage after_next{};

    BLITZAR_CHECK(codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(codec.Decode(MakeView(replay.base), buffers, MakeTarget(after_current)) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(SameState(replay.current, after_current));
    BLITZAR_CHECK(
        codec.Encode(MakeView(after_current), MakeView(replay.next), buffers) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(codec.Decode(MakeView(after_current), buffers, MakeTarget(after_next)) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(SameState(replay.next, after_next));

    return 0;
}

[[nodiscard]] TimingEvidence MeasureEvidence(const blitzar_io::SnapshotDelta& codec,
    const ReplayCase& replay, blitzar_io::SnapshotDeltaBuffer& buffers) noexcept
{
    std::array<std::byte, RawBytes> reference{};
    std::array<std::byte, RawBytes> restored{};
    SnapshotStorage target{};

    (void)codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers);

    const auto reference_write = [&]() noexcept {
        std::copy(buffers.current.begin(), buffers.current.end(), reference.begin());

        return Checksum(std::span<const std::byte>(reference));
    };

    const auto reference_read = [&]() noexcept {
        std::copy(reference.begin(), reference.end(), restored.begin());

        return Checksum(std::span<const std::byte>(restored));
    };

    const auto delta_write = [&]() noexcept {
        return static_cast<std::uint64_t>(
            codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers));
    };

    const auto delta_read = [&]() noexcept {
        return static_cast<std::uint64_t>(
            codec.Decode(MakeView(replay.base), buffers, MakeTarget(target)));
    };

    return {Measure(reference_write, RepeatCount), Measure(reference_read, RepeatCount),
        Measure(delta_write, RepeatCount), Measure(delta_read, RepeatCount)};
}

} // namespace

int main()
{
    ReplayCase replay{};

    FillStorage(replay.base, 1000, 0.0);
    FillStorage(replay.current, 1000, 0.000001);
    FillStorage(replay.next, 1000, 0.000002);

    blitzar_io::SnapshotDelta codec(ParticleCount);
    std::array<std::byte, RawBytes> base_bytes{};
    std::array<std::byte, RawBytes> current_bytes{};
    std::array<std::byte, RawBytes> decoded_bytes{};
    std::array<std::byte, EncodedBytes> encoded_bytes{};
    blitzar_io::SnapshotDeltaBuffer buffers{std::span<std::byte>(base_bytes),
        std::span<std::byte>(current_bytes), std::span<std::byte>(decoded_bytes),
        std::span<std::byte>(encoded_bytes), 0};

    BLITZAR_CHECK(codec.RawPayloadBytes(ParticleCount) == RawBytes);
    BLITZAR_CHECK(codec.MaxEncodedBytes(ParticleCount) == EncodedBytes);
    BLITZAR_CHECK(codec.WorkspaceBytes(ParticleCount) == 3 * RawBytes);
    BLITZAR_CHECK(CheckRoundTrip(codec, replay, buffers) == 0);
    BLITZAR_CHECK(CheckDeterminism(codec, replay, buffers) == 0);
    BLITZAR_CHECK(CheckCorruption(codec, replay, buffers) == 0);
    BLITZAR_CHECK(CheckReplay(codec, replay, buffers) == 0);

    BLITZAR_CHECK(codec.Encode(MakeView(replay.base), MakeView(replay.current), buffers) ==
                  BLITZAR_STATUS_OK);

    const TimingEvidence evidence = MeasureEvidence(codec, replay, buffers);
    const std::uint64_t checksum = Checksum(std::span<const std::byte>(buffers.current));
    const std::size_t ratio_ppm = buffers.encoded_size * 1000000U / RawBytes;

    std::cout << "delta-evidence backend=cpu codec=xor-rle-v1 scope=snapshot-transport-payload"
              << " raw_bytes=" << RawBytes << " encoded_bytes=" << buffers.encoded_size
              << " ratio_ppm=" << ratio_ppm << " reference_write_ns=" << evidence.reference_write_ns
              << " reference_read_ns=" << evidence.reference_read_ns
              << " delta_write_ns=" << evidence.delta_write_ns
              << " delta_read_ns=" << evidence.delta_read_ns
              << " workspace_bytes=" << codec.WorkspaceBytes(ParticleCount)
              << " keyframe_interval=" << KeyframeInterval
              << " random_access=index-replay checksum=" << checksum
              << " deterministic=1 corruption_rejected=1 transactional=1 transport=1"
              << " fallback=binary-snapshot-codec\n";

    return 0;
}
