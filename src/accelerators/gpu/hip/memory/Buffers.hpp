#ifndef BLITZAR_ACCELERATORS_GPU_HIP_MEMORY_BUFFERS_HPP
#define BLITZAR_ACCELERATORS_GPU_HIP_MEMORY_BUFFERS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

namespace blitzar_hip {

class Buffers final {
public:
    Buffers() noexcept;
    ~Buffers() noexcept;

    Buffers(const Buffers&) = delete;
    Buffers& operator=(const Buffers&) = delete;

    Buffers(Buffers&& other) noexcept;
    Buffers& operator=(Buffers&& other) noexcept;

    [[nodiscard]] bool IsAvailable() const noexcept;
    void Disable() noexcept;
    [[nodiscard]] bool Ensure(
        std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept;

    [[nodiscard]] std::uintptr_t Stream() const noexcept;
    [[nodiscard]] std::uintptr_t HostParticle(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t DeviceParticle(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t HostSource(std::size_t index) const noexcept;
    [[nodiscard]] std::uintptr_t DeviceSource(std::size_t index) const noexcept;
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

} // namespace blitzar_hip

#endif
