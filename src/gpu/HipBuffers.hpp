#ifndef BLITZAR_GPU_HIP_BUFFERS_HPP
#define BLITZAR_GPU_HIP_BUFFERS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

namespace blitzar_gpu {

class HipBuffers final {
public:
    HipBuffers() noexcept;
    ~HipBuffers() noexcept;

    HipBuffers(const HipBuffers&) = delete;
    HipBuffers& operator=(const HipBuffers&) = delete;

    HipBuffers(HipBuffers&& other) noexcept;
    HipBuffers& operator=(HipBuffers&& other) noexcept;

    [[nodiscard]] bool IsAvailable() const noexcept;
    void Disable() noexcept;
    [[nodiscard]] bool Ensure(std::size_t particle_count, std::size_t cell_count) noexcept;

    [[nodiscard]] std::uintptr_t Stream() const noexcept;
    [[nodiscard]] std::uintptr_t HostParticle(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t DeviceParticle(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t HostForce(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t DeviceForce(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t HostError() const noexcept;
    [[nodiscard]] std::uintptr_t DeviceError() const noexcept;
    [[nodiscard]] std::uintptr_t HostCells() const noexcept;
    [[nodiscard]] std::uintptr_t DeviceCells() const noexcept;
    [[nodiscard]] std::uintptr_t HostIndices() const noexcept;
    [[nodiscard]] std::uintptr_t DeviceIndices() const noexcept;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace blitzar_gpu

#endif
