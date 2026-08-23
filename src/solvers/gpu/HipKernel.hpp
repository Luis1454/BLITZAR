#ifndef BLITZAR_SOLVERS_GPU_HIP_KERNEL_HPP
#define BLITZAR_SOLVERS_GPU_HIP_KERNEL_HPP

#include "core/Solver.hpp"

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

struct KernelRuntime final {
    std::uintptr_t error_address{};
    std::uintptr_t stream{};
};

struct KernelPhysics final {
    double gravitational_constant{};
    double softening{};
};

struct TreeAddresses final {
    std::uintptr_t cells{};
    std::size_t cell_count{};
    std::uintptr_t indices{};
};

struct DirectLaunchRequest final {
    DeviceParticleAddresses addresses;
    std::size_t target_count{};
    blitzar_core::ForceRange range;
    KernelPhysics physics;
    KernelRuntime runtime;
};

struct BarnesHutLaunchRequest final {
    DeviceParticleAddresses addresses;
    std::size_t target_count{};
    std::size_t source_count{};
    double opening_angle{};
    TreeAddresses tree;
    KernelPhysics physics;
    std::size_t max_depth{};
    KernelRuntime runtime;
};

[[nodiscard]] blitzar_status LaunchDirect(const DirectLaunchRequest& request) noexcept;

[[nodiscard]] blitzar_status LaunchBarnesHut(const BarnesHutLaunchRequest& request) noexcept;

} // namespace blitzar_gpu_detail

#endif
