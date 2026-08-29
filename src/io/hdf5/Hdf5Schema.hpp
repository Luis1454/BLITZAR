#ifndef BLITZAR_IO_HDF5_HDF5_SCHEMA_HPP
#define BLITZAR_IO_HDF5_HDF5_SCHEMA_HPP

#include "core/CoreSnapshot.hpp"
#include "io/snapshot/SnapshotChecksum.hpp"

#include <cstdint>
#include <span>
#include <string_view>

#if defined(BLITZAR_HAS_HDF5)
#include <hdf5.h>
#endif

namespace blitzar_io {

inline constexpr std::uint32_t Hdf5SchemaVersion = 1;
inline constexpr std::string_view Hdf5ParticleGroupName = "particles";

template <typename Value>
void AddHdf5Values(SnapshotChecksum& checksum, std::span<const Value> values) noexcept
{
    for (const Value value : values) {
        checksum.Add(value);
    }
}

[[nodiscard]] inline std::uint64_t Hdf5PayloadChecksum(
    blitzar_core::SnapshotPayloadView payload) noexcept
{
    SnapshotChecksum checksum;

    AddHdf5Values(checksum, payload.ids);
    AddHdf5Values(checksum, payload.position_x);
    AddHdf5Values(checksum, payload.position_y);
    AddHdf5Values(checksum, payload.position_z);
    AddHdf5Values(checksum, payload.velocity_x);
    AddHdf5Values(checksum, payload.velocity_y);
    AddHdf5Values(checksum, payload.velocity_z);
    AddHdf5Values(checksum, payload.mass);

    return checksum.Value();
}

#if defined(BLITZAR_HAS_HDF5)

template <herr_t (*Close)(hid_t)> class Hdf5Resource final {
public:
    explicit Hdf5Resource(hid_t id) noexcept : id_(id) {}

    ~Hdf5Resource()
    {
        Reset();
    }

    Hdf5Resource(const Hdf5Resource&) = delete;
    Hdf5Resource& operator=(const Hdf5Resource&) = delete;

    Hdf5Resource(Hdf5Resource&& other) noexcept : id_(other.id_)
    {
        other.id_ = H5I_INVALID_HID;
    }

    Hdf5Resource& operator=(Hdf5Resource&& other) noexcept
    {
        if (this != &other) {
            Reset();

            id_ = other.id_;
            other.id_ = H5I_INVALID_HID;
        }

        return *this;
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        return id_ != H5I_INVALID_HID;
    }

    [[nodiscard]] hid_t Get() const noexcept
    {
        return id_;
    }

private:
    void Reset() noexcept
    {
        if (IsValid()) {
            (void)Close(id_);

            id_ = H5I_INVALID_HID;
        }
    }

    hid_t id_;
};

class Hdf5ErrorScope final {
public:
    Hdf5ErrorScope() noexcept
    {
        (void)H5Eget_auto2(H5E_DEFAULT, &handler_, &data_);
        (void)H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
    }

    ~Hdf5ErrorScope()
    {
        (void)H5Eset_auto2(H5E_DEFAULT, handler_, data_);
    }

    Hdf5ErrorScope(const Hdf5ErrorScope&) = delete;
    Hdf5ErrorScope& operator=(const Hdf5ErrorScope&) = delete;

private:
    H5E_auto2_t handler_{};
    void* data_{};
};

using Hdf5File = Hdf5Resource<&H5Fclose>;
using Hdf5Group = Hdf5Resource<&H5Gclose>;
using Hdf5Dataset = Hdf5Resource<&H5Dclose>;
using Hdf5Dataspace = Hdf5Resource<&H5Sclose>;
using Hdf5Attribute = Hdf5Resource<&H5Aclose>;
using Hdf5Property = Hdf5Resource<&H5Pclose>;

struct Hdf5AttributeSpec final {
    std::string_view name;
    hid_t file_type;
    hid_t memory_type;
};

struct Hdf5DatasetSpec final {
    std::string_view name;
    hid_t file_type;
    hid_t memory_type;
};

[[nodiscard]] inline Hdf5AttributeSpec Hdf5SchemaAttribute() noexcept
{
    return {"schema_version", H5T_STD_U32LE, H5T_NATIVE_UINT32};
}

[[nodiscard]] inline Hdf5AttributeSpec Hdf5ChecksumAttribute() noexcept
{
    return {"payload_checksum", H5T_STD_U64LE, H5T_NATIVE_UINT64};
}

[[nodiscard]] inline Hdf5AttributeSpec Hdf5HeaderAttribute(
    blitzar_core::SnapshotHeaderField field) noexcept
{
    switch (field) {
    case blitzar_core::SnapshotHeaderField::Magic:

        return {"magic", H5T_STD_U32LE, H5T_NATIVE_UINT32};

    case blitzar_core::SnapshotHeaderField::Version:

        return {"version", H5T_STD_U16LE, H5T_NATIVE_UINT16};

    case blitzar_core::SnapshotHeaderField::ScalarBytes:

        return {"scalar_bytes", H5T_STD_U16LE, H5T_NATIVE_UINT16};

    case blitzar_core::SnapshotHeaderField::ParticleCount:

        return {"particle_count", H5T_STD_U64LE, H5T_NATIVE_UINT64};

    case blitzar_core::SnapshotHeaderField::Step:

        return {"step", H5T_STD_U64LE, H5T_NATIVE_UINT64};

    case blitzar_core::SnapshotHeaderField::Time:

        return {"time", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotHeaderField::RankCount:

        return {"rank_count", H5T_STD_U32LE, H5T_NATIVE_UINT32};

    case blitzar_core::SnapshotHeaderField::RankIndex:

        return {"rank_index", H5T_STD_U32LE, H5T_NATIVE_UINT32};

    case blitzar_core::SnapshotHeaderField::Endianness:

        return {"endianness", H5T_STD_U8LE, H5T_NATIVE_UINT8};

    case blitzar_core::SnapshotHeaderField::Distribution:

        return {"distribution", H5T_STD_U8LE, H5T_NATIVE_UINT8};

    case blitzar_core::SnapshotHeaderField::IdPolicy:

        return {"id_policy", H5T_STD_U8LE, H5T_NATIVE_UINT8};

    default:

        return {"", H5I_INVALID_HID, H5I_INVALID_HID};
    }
}

[[nodiscard]] inline Hdf5DatasetSpec Hdf5ParticleDataset(blitzar_core::SnapshotField field) noexcept
{
    switch (field) {
    case blitzar_core::SnapshotField::Ids:

        return {"ids", H5T_STD_U64LE, H5T_NATIVE_UINT64};

    case blitzar_core::SnapshotField::PositionX:

        return {"position_x", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotField::PositionY:

        return {"position_y", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotField::PositionZ:

        return {"position_z", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotField::VelocityX:

        return {"velocity_x", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotField::VelocityY:

        return {"velocity_y", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotField::VelocityZ:

        return {"velocity_z", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    case blitzar_core::SnapshotField::Mass:

        return {"mass", H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE};

    default:

        return {"", H5I_INVALID_HID, H5I_INVALID_HID};
    }
}

#endif

} // namespace blitzar_io

#endif
