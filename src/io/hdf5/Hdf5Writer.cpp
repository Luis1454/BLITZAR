#include "io/hdf5/Hdf5Writer.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(BLITZAR_HAS_HDF5)
#include "io/hdf5/Hdf5Schema.hpp"
#endif

namespace blitzar_io {

namespace {

[[nodiscard]] blitzar_status ValidateInput(const std::filesystem::path& path,
    blitzar_core::SnapshotFrameView frame, std::size_t max_particle_count) noexcept
{
    if (path.empty() || frame.header.particle_count > max_particle_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return frame.Validate();
}

#if defined(BLITZAR_HAS_HDF5)

template <typename Value>
[[nodiscard]] bool WriteAttribute(hid_t file, Hdf5AttributeSpec spec, Value value) noexcept
{
    Hdf5Dataspace space(H5Screate(H5S_SCALAR));

    if (!space.IsValid()) {
        return false;
    }

    Hdf5Attribute attribute(
        H5Acreate2(file, spec.name.data(), spec.file_type, space.Get(), H5P_DEFAULT, H5P_DEFAULT));

    return attribute.IsValid() && H5Awrite(attribute.Get(), spec.memory_type, &value) >= 0;
}

[[nodiscard]] bool WriteHeaderAttributes(
    hid_t file, const blitzar_core::SnapshotHeader& header, std::uint64_t checksum) noexcept
{
    using Field = blitzar_core::SnapshotHeaderField;

    return WriteAttribute(file, Hdf5SchemaAttribute(), Hdf5SchemaVersion) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::Magic), header.magic) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::Version),
               static_cast<std::uint16_t>(header.version)) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::ScalarBytes), header.scalar_bytes) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::ParticleCount), header.particle_count) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::Step), header.step) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::Time), header.time) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::RankCount), header.rank_count) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::RankIndex), header.rank_index) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::Endianness),
               static_cast<std::uint8_t>(header.endianness)) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::Distribution),
               static_cast<std::uint8_t>(header.distribution)) &&
           WriteAttribute(file, Hdf5HeaderAttribute(Field::IdPolicy),
               static_cast<std::uint8_t>(header.id_policy)) &&
           WriteAttribute(file, Hdf5ChecksumAttribute(), checksum);
}

template <typename Value>
[[nodiscard]] bool WriteDataset(hid_t group, Hdf5DatasetSpec spec, std::span<const Value> values,
    hid_t creation_property) noexcept
{
    const hsize_t dimensions[] = {static_cast<hsize_t>(values.size())};
    Hdf5Dataspace space(H5Screate_simple(1, dimensions, nullptr));

    if (!space.IsValid()) {
        return false;
    }

    Hdf5Dataset dataset(H5Dcreate2(group, spec.name.data(), spec.file_type, space.Get(),
        H5P_DEFAULT, creation_property, H5P_DEFAULT));

    if (!dataset.IsValid()) {
        return false;
    }

    return values.empty() || H5Dwrite(dataset.Get(), spec.memory_type, H5S_ALL, H5S_ALL,
                                 H5P_DEFAULT, values.data()) >= 0;
}

[[nodiscard]] bool WritePayload(
    hid_t group, blitzar_core::SnapshotPayloadView payload, hid_t creation_property) noexcept
{
    using Field = blitzar_core::SnapshotField;

    return WriteDataset(group, Hdf5ParticleDataset(Field::Ids), payload.ids, creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::PositionX), payload.position_x,
               creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::PositionY), payload.position_y,
               creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::PositionZ), payload.position_z,
               creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::VelocityX), payload.velocity_x,
               creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::VelocityY), payload.velocity_y,
               creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::VelocityZ), payload.velocity_z,
               creation_property) &&
           WriteDataset(group, Hdf5ParticleDataset(Field::Mass), payload.mass, creation_property);
}

[[nodiscard]] Hdf5Property CreateObjectProperty(hid_t property_class) noexcept
{
    Hdf5Property property(H5Pcreate(property_class));

    if (!property.IsValid() || H5Pset_obj_track_times(property.Get(), 0) < 0) {
        return Hdf5Property(H5I_INVALID_HID);
    }

    return property;
}

[[nodiscard]] blitzar_status WriteFile(
    const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame)
{
    Hdf5ErrorScope error_scope;
    const std::string file_name = path.string();
    Hdf5File file(H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));

    if (!file.IsValid()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    Hdf5Property group_property = CreateObjectProperty(H5P_GROUP_CREATE);
    Hdf5Property dataset_property = CreateObjectProperty(H5P_DATASET_CREATE);

    if (!group_property.IsValid() || !dataset_property.IsValid()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const std::uint64_t checksum = Hdf5PayloadChecksum(frame.payload);

    if (!WriteHeaderAttributes(file.Get(), frame.header, checksum)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    Hdf5Group group(H5Gcreate2(
        file.Get(), Hdf5ParticleGroupName.data(), H5P_DEFAULT, group_property.Get(), H5P_DEFAULT));

    if (!group.IsValid() || !WritePayload(group.Get(), frame.payload, dataset_property.Get()) ||
        H5Fflush(file.Get(), H5F_SCOPE_GLOBAL) < 0) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status WriteAtomicFile(
    const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame)
{
    std::error_code status_error;

    if (std::filesystem::exists(path, status_error) || status_error) {
        return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::filesystem::path temporary = path;

    temporary += ".tmp";

    status_error.clear();

    if (std::filesystem::exists(temporary, status_error) || status_error) {
        return status_error ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status write_status = WriteFile(temporary, frame);

    if (write_status != BLITZAR_STATUS_OK) {
        std::error_code cleanup_error;

        std::filesystem::remove(temporary, cleanup_error);

        return write_status;
    }

    std::error_code rename_error;

    std::filesystem::rename(temporary, path, rename_error);

    if (rename_error) {
        std::error_code cleanup_error;

        std::filesystem::remove(temporary, cleanup_error);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
}

#endif

} // namespace

Hdf5Writer::Hdf5Writer(std::size_t max_particle_count) noexcept
    : max_particle_count_(std::min(
          max_particle_count, static_cast<std::size_t>(blitzar_core::SnapshotMaxParticleCount)))
{
}

bool Hdf5Writer::IsAvailable() noexcept
{
#if defined(BLITZAR_HAS_HDF5)
    return true;
#else
    return false;
#endif
}

blitzar_status Hdf5Writer::Write(
    const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame) const
{
    const blitzar_status input_status = ValidateInput(path, frame, max_particle_count_);

    if (input_status != BLITZAR_STATUS_OK) {
        return input_status;
    }

#if defined(BLITZAR_HAS_HDF5)

    try {
        return WriteFile(path, frame);
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

    return BLITZAR_STATUS_UNSUPPORTED;
#endif
}

blitzar_status Hdf5Writer::WriteAtomic(
    const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame) const
{
    const blitzar_status input_status = ValidateInput(path, frame, max_particle_count_);

    if (input_status != BLITZAR_STATUS_OK) {
        return input_status;
    }

#if defined(BLITZAR_HAS_HDF5)

    try {
        return WriteAtomicFile(path, frame);
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

    return BLITZAR_STATUS_UNSUPPORTED;
#endif
}

} // namespace blitzar_io
