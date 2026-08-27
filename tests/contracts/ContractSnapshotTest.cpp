#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <span>

namespace {

struct SnapshotStorage final {
    std::array<std::uint64_t, 2> ids{0, 1};
    std::array<blitzar_core::Scalar, 2> position_x{1.0, 2.0};
    std::array<blitzar_core::Scalar, 2> position_y{3.0, 4.0};
    std::array<blitzar_core::Scalar, 2> position_z{5.0, 6.0};
    std::array<blitzar_core::Scalar, 2> velocity_x{0.1, 0.2};
    std::array<blitzar_core::Scalar, 2> velocity_y{0.3, 0.4};
    std::array<blitzar_core::Scalar, 2> velocity_z{0.5, 0.6};
    std::array<blitzar_core::Scalar, 2> mass{1.0, 2.0};
};

[[nodiscard]] blitzar_core::SnapshotPayloadView MakePayload(const SnapshotStorage& storage) noexcept
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

} // namespace

int main()
{
    SnapshotStorage storage{};

    blitzar_core::SnapshotHeader header{};

    header.particle_count = storage.ids.size();
    header.step = 7;
    header.time = 0.25;

    BLITZAR_CHECK(header.Validate() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(header.IsCompatible());

    const blitzar_core::SnapshotPayloadView payload = MakePayload(storage);
    const blitzar_core::SnapshotFrameView frame{header, payload};

    BLITZAR_CHECK(frame.Validate() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_core::SnapshotHeaderFieldOrder.size() == 10);
    BLITZAR_CHECK(blitzar_core::SnapshotPayloadOrder.size() == 8);
    BLITZAR_CHECK(blitzar_core::SnapshotPayloadOrder[0] == blitzar_core::SnapshotField::Ids);
    BLITZAR_CHECK(blitzar_core::SnapshotPayloadOrder[7] == blitzar_core::SnapshotField::Mass);

    header.time = std::numeric_limits<blitzar_core::Scalar>::quiet_NaN();

    BLITZAR_CHECK(header.Validate() == BLITZAR_STATUS_INVALID_ARGUMENT);

    header.time = 0.25;
    header.scalar_bytes = 4;

    BLITZAR_CHECK(header.Validate() == BLITZAR_STATUS_UNSUPPORTED);

    header.scalar_bytes = blitzar_core::SnapshotScalarBytes;
    header.version = static_cast<blitzar_core::SnapshotVersion>(99);

    BLITZAR_CHECK(header.Validate() == BLITZAR_STATUS_UNSUPPORTED);

    header.version = blitzar_core::SnapshotVersion::V1;
    storage.ids[1] = 9;

    const blitzar_core::SnapshotFrameView invalid_id_frame{header, MakePayload(storage)};

    BLITZAR_CHECK(invalid_id_frame.Validate() == BLITZAR_STATUS_INVALID_ARGUMENT);

    storage.ids[1] = 1;

    blitzar_core::SnapshotPayloadView mismatched_payload = MakePayload(storage);

    mismatched_payload.position_x = mismatched_payload.position_x.first(1);

    const blitzar_core::SnapshotFrameView mismatched_frame{header, mismatched_payload};

    BLITZAR_CHECK(mismatched_frame.Validate() == BLITZAR_STATUS_INVALID_ARGUMENT);

    storage.mass[0] = std::numeric_limits<blitzar_core::Scalar>::infinity();

    const blitzar_core::SnapshotFrameView non_finite_frame{header, MakePayload(storage)};

    BLITZAR_CHECK(non_finite_frame.Validate() == BLITZAR_STATUS_INVALID_ARGUMENT);

    header.distribution = blitzar_core::SnapshotDistribution::Sharded;
    header.id_policy = blitzar_core::SnapshotIdPolicy::GlobalStable;
    header.rank_count = 2;

    BLITZAR_CHECK(header.Validate() == BLITZAR_STATUS_UNSUPPORTED);

    return 0;
}
