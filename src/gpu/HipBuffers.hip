#include "gpu/HipBuffers.hpp"

#include "solvers/gpu/HipKernel.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>

namespace blitzar_gpu {

namespace {

class PinnedAllocation final {
public:
    PinnedAllocation() = default;
    PinnedAllocation(const PinnedAllocation&) = delete;
    PinnedAllocation& operator=(const PinnedAllocation&) = delete;
    ~PinnedAllocation()
    {
        Release();
    }

    [[nodiscard]] bool Resize(std::size_t bytes) noexcept
    {
        if (bytes == bytes_) {
            return true;
        }
        Release();
        if (bytes == 0) {
            return true;
        }
        if (hipHostMalloc(&data_, bytes, hipHostMallocDefault) != hipSuccess) {
            return false;
        }
        bytes_ = bytes;
        return true;
    }

    void Release() noexcept
    {
        if (data_ != nullptr) {
            (void)hipHostFree(data_);
            data_ = nullptr;
        }
        bytes_ = 0;
    }

    [[nodiscard]] std::uintptr_t Address() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(data_);
    }

private:
    void* data_{};
    std::size_t bytes_{};
};

class DeviceAllocation final {
public:
    DeviceAllocation() = default;
    DeviceAllocation(const DeviceAllocation&) = delete;
    DeviceAllocation& operator=(const DeviceAllocation&) = delete;
    ~DeviceAllocation()
    {
        Release();
    }

    [[nodiscard]] bool Resize(std::size_t bytes) noexcept
    {
        if (bytes == bytes_) {
            return true;
        }
        Release();
        if (bytes == 0) {
            return true;
        }
        if (hipMalloc(&data_, bytes) != hipSuccess) {
            return false;
        }
        bytes_ = bytes;
        return true;
    }

    void Release() noexcept
    {
        if (data_ != nullptr) {
            (void)hipFree(data_);
            data_ = nullptr;
        }
        bytes_ = 0;
    }

    [[nodiscard]] std::uintptr_t Address() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(data_);
    }

private:
    void* data_{};
    std::size_t bytes_{};
};

}  // namespace

struct HipBuffers::Impl final {
    hipStream_t stream{};
    bool available{};
    std::array<PinnedAllocation, 4> host_particles;
    std::array<DeviceAllocation, 4> device_particles;
    std::array<PinnedAllocation, 3> host_forces;
    std::array<DeviceAllocation, 3> device_forces;
    PinnedAllocation host_error;
    DeviceAllocation device_error;
    PinnedAllocation host_cells;
    DeviceAllocation device_cells;
    PinnedAllocation host_indices;
    DeviceAllocation device_indices;

    Impl() noexcept
    {
        int device_count = 0;
        if (hipGetDeviceCount(&device_count) != hipSuccess ||
            device_count <= 0 || hipSetDevice(0) != hipSuccess ||
            hipStreamCreateWithFlags(&stream, hipStreamNonBlocking) !=
                hipSuccess) {
            return;
        }
        available = true;
    }

    ~Impl()
    {
        if (stream != nullptr) {
            (void)hipStreamSynchronize(stream);
            (void)hipStreamDestroy(stream);
            stream = nullptr;
        }
    }

    [[nodiscard]] bool Ensure(
        std::size_t particle_count,
        std::size_t cell_count) noexcept
    {
        const std::size_t maximum = std::numeric_limits<std::size_t>::max();
        if (particle_count > maximum / sizeof(double) ||
            cell_count > maximum / sizeof(blitzar_gpu_detail::GpuCell) ||
            particle_count > maximum / sizeof(std::uint64_t)) {
            return false;
        }
        const std::size_t scalar_bytes = particle_count * sizeof(double);
        const std::size_t cell_bytes =
            cell_count * sizeof(blitzar_gpu_detail::GpuCell);
        const std::size_t index_bytes = particle_count * sizeof(std::uint64_t);
        for (PinnedAllocation& buffer : host_particles) {
            if (!buffer.Resize(scalar_bytes)) {
                return false;
            }
        }
        for (DeviceAllocation& buffer : device_particles) {
            if (!buffer.Resize(scalar_bytes)) {
                return false;
            }
        }
        for (PinnedAllocation& buffer : host_forces) {
            if (!buffer.Resize(scalar_bytes)) {
                return false;
            }
        }
        for (DeviceAllocation& buffer : device_forces) {
            if (!buffer.Resize(scalar_bytes)) {
                return false;
            }
        }
        return host_error.Resize(sizeof(int)) &&
               device_error.Resize(sizeof(int)) &&
               host_cells.Resize(cell_bytes) &&
               device_cells.Resize(cell_bytes) &&
               host_indices.Resize(index_bytes) &&
               device_indices.Resize(index_bytes);
    }
};

HipBuffers::HipBuffers() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    } catch (const std::bad_alloc&) {
        impl_.reset();
    }
}

HipBuffers::~HipBuffers() noexcept = default;

HipBuffers::HipBuffers(HipBuffers&& other) noexcept = default;

HipBuffers& HipBuffers::operator=(HipBuffers&& other) noexcept = default;

bool HipBuffers::IsAvailable() const noexcept
{
    return impl_ != nullptr && impl_->available;
}

void HipBuffers::Disable() noexcept
{
    if (impl_ != nullptr) {
        impl_->available = false;
    }
}

bool HipBuffers::Ensure(
    std::size_t particle_count,
    std::size_t cell_count) noexcept
{
    return impl_ != nullptr && impl_->available &&
           impl_->Ensure(particle_count, cell_count);
}

std::uintptr_t HipBuffers::Stream() const noexcept
{
    return impl_ == nullptr
               ? 0
               : reinterpret_cast<std::uintptr_t>(impl_->stream);
}

std::uintptr_t HipBuffers::HostParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_particles.size()
               ? 0
               : impl_->host_particles[index].Address();
}

std::uintptr_t HipBuffers::DeviceParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_particles.size()
               ? 0
               : impl_->device_particles[index].Address();
}

std::uintptr_t HipBuffers::HostForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_forces.size()
               ? 0
               : impl_->host_forces[index].Address();
}

std::uintptr_t HipBuffers::DeviceForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_forces.size()
               ? 0
               : impl_->device_forces[index].Address();
}

std::uintptr_t HipBuffers::HostError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_error.Address();
}

std::uintptr_t HipBuffers::DeviceError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_error.Address();
}

std::uintptr_t HipBuffers::HostCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_cells.Address();
}

std::uintptr_t HipBuffers::DeviceCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_cells.Address();
}

std::uintptr_t HipBuffers::HostIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_indices.Address();
}

std::uintptr_t HipBuffers::DeviceIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_indices.Address();
}

}  // namespace blitzar_gpu
