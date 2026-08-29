#include "io/hdf5/Hdf5Reader.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(BLITZAR_HAS_HDF5)
#include "io/hdf5/Hdf5Schema.hpp"
#endif

namespace blitzar_io {

namespace {

#if defined(BLITZAR_HAS_HDF5)

struct Hdf5PayloadStorage final {
    std::vector<std::uint64_t> ids;
    std::vector<blitzar_core::Scalar> position_x;
    std::vector<blitzar_core::Scalar> position_y;
    std::vector<blitzar_core::Scalar> position_z;
    std::vector<blitzar_core::Scalar> velocity_x;
    std::vector<blitzar_core::Scalar> velocity_y;
    std::vector<blitzar_core::Scalar> velocity_z;
    std::vector<blitzar_core::Scalar> mass;

    void Resize(std::size_t count)
    {
        ids.resize(count);
        position_x.resize(count);
        position_y.resize(count);
        position_z.resize(count);
        velocity_x.resize(count);
        velocity_y.resize(count);
        velocity_z.resize(count);
        mass.resize(count);
    }

    [[nodiscard]] blitzar_core::SnapshotPayloadView View() const noexcept
    {
        return {std::span<const std::uint64_t>(ids),
            std::span<const blitzar_core::Scalar>(position_x),
            std::span<const blitzar_core::Scalar>(position_y),
            std::span<const blitzar_core::Scalar>(position_z),
            std::span<const blitzar_core::Scalar>(velocity_x),
            std::span<const blitzar_core::Scalar>(velocity_y),
            std::span<const blitzar_core::Scalar>(velocity_z),
            std::span<const blitzar_core::Scalar>(mass)};
    }
};

template <typename Value>
[[nodiscard]] bool ReadAttribute(hid_t file, Hdf5AttributeSpec spec, Value& value) noexcept
{
    Hdf5Attribute attribute(H5Aopen(file, spec.name.data(), H5P_DEFAULT));

    return attribute.IsValid() && H5Aread(attribute.Get(), spec.memory_type, &value) >= 0;
}

[[nodiscard]] blitzar_status ReadHeader(hid_t file, blitzar_core::SnapshotHeader& header) noexcept
{
    using Field = blitzar_core::SnapshotHeaderField;

    std::uint32_t schema_version{};
    std::uint32_t magic{};
    std::uint16_t version{};
    std::uint16_t scalar_bytes{};
    std::uint64_t particle_count{};
    std::uint64_t step{};
    blitzar_core::Scalar time{};
    std::uint32_t rank_count{};
    std::uint32_t rank_index{};
    std::uint8_t endianness{};
    std::uint8_t distribution{};
    std::uint8_t id_policy{};

    if (!ReadAttribute(file, Hdf5SchemaAttribute(), schema_version)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (schema_version != Hdf5SchemaVersion) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    if (!ReadAttribute(file, Hdf5HeaderAttribute(Field::Magic), magic) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::Version), version) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::ScalarBytes), scalar_bytes) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::ParticleCount), particle_count) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::Step), step) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::Time), time) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::RankCount), rank_count) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::RankIndex), rank_index) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::Endianness), endianness) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::Distribution), distribution) ||
        !ReadAttribute(file, Hdf5HeaderAttribute(Field::IdPolicy), id_policy)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_core::SnapshotHeader candidate{magic,
        static_cast<blitzar_core::SnapshotVersion>(version), scalar_bytes, particle_count, step,
        time, rank_count, rank_index, static_cast<blitzar_core::SnapshotEndianness>(endianness),
        static_cast<blitzar_core::SnapshotDistribution>(distribution),
        static_cast<blitzar_core::SnapshotIdPolicy>(id_policy)};

    const blitzar_status status = candidate.Validate();

    if (status == BLITZAR_STATUS_OK) {
        header = candidate;
    }

    return status;
}

template <typename Value>
[[nodiscard]] bool ReadDataset(hid_t group, Hdf5DatasetSpec spec, std::span<Value> values) noexcept
{
    Hdf5Dataset dataset(H5Dopen2(group, spec.name.data(), H5P_DEFAULT));

    if (!dataset.IsValid()) {
        return false;
    }

    Hdf5Dataspace space(H5Dget_space(dataset.Get()));

    if (!space.IsValid() || H5Sget_simple_extent_ndims(space.Get()) != 1) {
        return false;
    }

    hsize_t dimensions[1]{};

    if (H5Sget_simple_extent_dims(space.Get(), dimensions, nullptr) != 1 ||
        dimensions[0] != values.size()) {
        return false;
    }

    return values.empty() || H5Dread(dataset.Get(), spec.memory_type, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                                 values.data()) >= 0;
}

[[nodiscard]] bool ReadPayload(hid_t group, Hdf5PayloadStorage& storage) noexcept
{
    using Field = blitzar_core::SnapshotField;
    const blitzar_core::SnapshotPayloadView payload = storage.View();

    return ReadDataset(
               group, Hdf5ParticleDataset(Field::Ids), std::span<std::uint64_t>(storage.ids)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::PositionX),
               std::span<blitzar_core::Scalar>(storage.position_x)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::PositionY),
               std::span<blitzar_core::Scalar>(storage.position_y)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::PositionZ),
               std::span<blitzar_core::Scalar>(storage.position_z)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::VelocityX),
               std::span<blitzar_core::Scalar>(storage.velocity_x)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::VelocityY),
               std::span<blitzar_core::Scalar>(storage.velocity_y)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::VelocityZ),
               std::span<blitzar_core::Scalar>(storage.velocity_z)) &&
           ReadDataset(group, Hdf5ParticleDataset(Field::Mass),
               std::span<blitzar_core::Scalar>(storage.mass)) &&
           payload.HasMatchingCounts(storage.ids.size());
}

template <typename Value>
void CommitValues(std::span<const Value> source, std::span<Value> destination) noexcept
{
    std::copy(source.begin(), source.end(), destination.begin());
}

void CommitPayload(
    const Hdf5PayloadStorage& source, blitzar_core::SnapshotMutablePayloadView destination) noexcept
{
    CommitValues(std::span<const std::uint64_t>(source.ids), destination.ids);
    CommitValues(std::span<const blitzar_core::Scalar>(source.position_x), destination.position_x);
    CommitValues(std::span<const blitzar_core::Scalar>(source.position_y), destination.position_y);
    CommitValues(std::span<const blitzar_core::Scalar>(source.position_z), destination.position_z);
    CommitValues(std::span<const blitzar_core::Scalar>(source.velocity_x), destination.velocity_x);
    CommitValues(std::span<const blitzar_core::Scalar>(source.velocity_y), destination.velocity_y);
    CommitValues(std::span<const blitzar_core::Scalar>(source.velocity_z), destination.velocity_z);
    CommitValues(std::span<const blitzar_core::Scalar>(source.mass), destination.mass);
}

[[nodiscard]] blitzar_status ReadFile(const std::filesystem::path& path,
    blitzar_core::SnapshotHeader& header, blitzar_core::SnapshotMutablePayloadView payload,
    std::size_t max_particle_count)
{
    Hdf5ErrorScope error_scope;
    const std::string file_name = path.string();
    Hdf5File file(H5Fopen(file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));

    if (!file.IsValid()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    blitzar_core::SnapshotHeader candidate{};
    const blitzar_status header_status = ReadHeader(file.Get(), candidate);

    if (header_status != BLITZAR_STATUS_OK) {
        return header_status;
    }

    if (candidate.particle_count > max_particle_count ||
        !payload.HasCapacity(candidate.particle_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    Hdf5PayloadStorage storage;

    storage.Resize(static_cast<std::size_t>(candidate.particle_count));

    Hdf5Group group(H5Gopen2(file.Get(), Hdf5ParticleGroupName.data(), H5P_DEFAULT));

    if (!group.IsValid() || !ReadPayload(group.Get(), storage)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_core::SnapshotPayloadView source = storage.View();
    const blitzar_core::SnapshotFrameView frame{candidate, source};

    if (frame.Validate() != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::uint64_t expected_checksum{};

    if (!ReadAttribute(file.Get(), Hdf5ChecksumAttribute(), expected_checksum) ||
        expected_checksum != Hdf5PayloadChecksum(source)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    CommitPayload(storage, payload);

    header = candidate;

    return BLITZAR_STATUS_OK;
}

#endif

} // namespace

Hdf5Reader::Hdf5Reader(std::size_t max_particle_count) noexcept
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount)))
{
}

bool Hdf5Reader::IsAvailable() noexcept
{
#if defined(BLITZAR_HAS_HDF5)
    return true;
#else
    return false;
#endif
}

blitzar_status Hdf5Reader::Read(const std::filesystem::path& path,
    blitzar_core::SnapshotHeader& header, blitzar_core::SnapshotMutablePayloadView payload) const
{
    if (path.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

#if defined(BLITZAR_HAS_HDF5)

    try {
        return ReadFile(path, header, payload, max_particle_count_);
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::filesystem::filesystem_error&) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
#else

    (void)header;
    (void)payload;

    return BLITZAR_STATUS_UNSUPPORTED;
#endif
}

} // namespace blitzar_io
