#ifndef BLITZAR_SOLVERS_GPU_HIP_KERNEL_HPP
#define BLITZAR_SOLVERS_GPU_HIP_KERNEL_HPP

#include <blitzar/blitzar.h>

#include <cstddef>
#include <cstdint>

namespace blitzar_gpu_detail {

struct DeviceParticleAddresses final {
    std::uintptr_t position_x{};
    std::uintptr_t position_y{};
    std::uintptr_t position_z{};
    std::uintptr_t mass{};
    std::uintptr_t force_x{};
    std::uintptr_t force_y{};
    std::uintptr_t force_z{};
};

struct GpuCell final {
    double center_x{};
    double center_y{};
    double center_z{};
    double center_of_mass_x{};
    double center_of_mass_y{};
    double center_of_mass_z{};
    double half_extent{};
    double mass{};
    std::uint64_t begin{};
    std::uint64_t count{};
    std::uint64_t children[8]{};
};

[[nodiscard]] blitzar_status LaunchDirect(
    DeviceParticleAddresses addresses,
    std::size_t target_count,
    std::size_t source_count,
    double gravitational_constant,
    double softening,
    std::uintptr_t error_address,
    std::uintptr_t stream) noexcept;

[[nodiscard]] blitzar_status LaunchBarnesHut(
    DeviceParticleAddresses addresses,
    std::size_t target_count,
    std::size_t source_count,
    std::uintptr_t cells,
    std::size_t cell_count,
    std::uintptr_t indices,
    double opening_angle,
    double gravitational_constant,
    double softening,
    std::size_t max_depth,
    std::uintptr_t error_address,
    std::uintptr_t stream) noexcept;

}  // namespace blitzar_gpu_detail

#endif
