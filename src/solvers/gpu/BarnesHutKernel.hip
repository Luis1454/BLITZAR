#include "solvers/gpu/HipKernel.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include <cstddef>
#include <cstdint>
#include <limits>

namespace blitzar_gpu_detail {

namespace {

constexpr unsigned int BlockSize = 128;
constexpr int MaxTraversalStack = 256;
constexpr std::uint64_t InvalidCell = std::numeric_limits<std::uint64_t>::max();

struct BarnesHutDeviceRequest final {
    const double* position_x;
    const double* position_y;
    const double* position_z;
    const double* mass;
    double* force_x;
    double* force_y;
    double* force_z;
    std::size_t target_count;
    std::size_t source_count;
    const GpuCell* cells;
    std::size_t cell_count;
    const std::uint64_t* indices;
    double opening_angle;
    double gravitational_constant;
    double softening;
    int* error;
};

__device__ void RecordError(int* error, int status) noexcept
{
    atomicMax(error, status);
}

__device__ int BroadcastWarpError(int value) noexcept
{
#if defined(__HIP_PLATFORM_NVIDIA__)
    return __shfl_sync(0xffffffffu, value, 0);
#else
    return __shfl(value, 0);
#endif
}

__device__ bool IsLeaf(const GpuCell& cell) noexcept
{
    for (unsigned int child = 0; child < 8; ++child) {
        if (cell.children[child] != InvalidCell) {
            return false;
        }
    }

    return true;
}

__device__ bool Contains(
    const GpuCell& cell, double position_x, double position_y, double position_z) noexcept
{
    return fabs(position_x - cell.center_x) <= cell.half_extent &&
           fabs(position_y - cell.center_y) <= cell.half_extent &&
           fabs(position_z - cell.center_z) <= cell.half_extent;
}

__global__ void BarnesHutKernel(BarnesHutDeviceRequest request)
{
    const double* position_x = request.position_x;
    const double* position_y = request.position_y;
    const double* position_z = request.position_z;
    const double* mass = request.mass;
    double* force_x = request.force_x;
    double* force_y = request.force_y;
    double* force_z = request.force_z;

    const std::size_t target_count = request.target_count;
    const std::size_t source_count = request.source_count;

    const GpuCell* cells = request.cells;

    const std::size_t cell_count = request.cell_count;
    const std::uint64_t* indices = request.indices;
    const double opening_angle = request.opening_angle;
    const double gravitational_constant = request.gravitational_constant;
    const double softening = request.softening;

    int* error = request.error;

    __shared__ int block_error;

    if (threadIdx.x == 0) {
        block_error = 0;
    }

    __syncthreads();

    const std::size_t target = static_cast<std::size_t>(blockIdx.x) * BlockSize + threadIdx.x;
    int local_error = 0;
    double acceleration_x = 0.0;
    double acceleration_y = 0.0;
    double acceleration_z = 0.0;
    const double softening_squared = softening * softening;
    std::uint64_t traversal_stack[MaxTraversalStack];
    int stack_size = 0;

    if (target < target_count) {
        const double target_x = position_x[target];
        const double target_y = position_y[target];
        const double target_z = position_z[target];
        const double target_mass = mass[target];

        if (!isfinite(target_x) || !isfinite(target_y) || !isfinite(target_z) ||
            !isfinite(target_mass) || target_mass < 0.0) {
            local_error = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        else {
            traversal_stack[stack_size++] = 0;
        }

        while (stack_size > 0 && local_error == 0) {
            const std::uint64_t cell_index = traversal_stack[--stack_size];

            if (cell_index >= cell_count) {
                local_error = BLITZAR_STATUS_INTERNAL_ERROR;

                break;
            }

            const GpuCell& cell = cells[cell_index];

            if (!isfinite(cell.mass) || cell.mass < 0.0 || cell.begin > source_count ||
                cell.count > source_count - cell.begin) {
                local_error = BLITZAR_STATUS_INTERNAL_ERROR;

                break;
            }
            if (cell.mass == 0.0) {
                continue;
            }

            const bool contains_target = Contains(cell, target_x, target_y, target_z);

            if (IsLeaf(cell)) {
                for (std::uint64_t offset = 0; offset < cell.count; ++offset) {
                    const std::uint64_t source = indices[cell.begin + offset];

                    if (source >= source_count) {
                        local_error = BLITZAR_STATUS_INTERNAL_ERROR;

                        break;
                    }

                    const double source_mass = mass[source];
                    const double source_x = position_x[source];
                    const double source_y = position_y[source];
                    const double source_z = position_z[source];

                    if (!isfinite(source_x) || !isfinite(source_y) || !isfinite(source_z) ||
                        !isfinite(source_mass) || source_mass < 0.0) {
                        local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                        break;
                    }
                    if (source == target || source_mass == 0.0) {
                        continue;
                    }

                    const double dx = source_x - target_x;
                    const double dy = source_y - target_y;
                    const double dz = source_z - target_z;
                    const double distance_squared = dx * dx + dy * dy + dz * dz;
                    const double softened_distance = distance_squared + softening_squared;

                    if (!isfinite(distance_squared) || !isfinite(softened_distance)) {
                        local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                        break;
                    }
                    if (softened_distance == 0.0) {
                        local_error = BLITZAR_STATUS_SINGULARITY;

                        break;
                    }

                    const double factor = gravitational_constant * source_mass /
                                          (softened_distance * sqrt(softened_distance));

                    if (!isfinite(factor)) {
                        local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                        break;
                    }

                    acceleration_x += factor * dx;
                    acceleration_y += factor * dy;
                    acceleration_z += factor * dz;
                }

                continue;
            }

            const double dx = cell.center_of_mass_x - target_x;
            const double dy = cell.center_of_mass_y - target_y;
            const double dz = cell.center_of_mass_z - target_z;
            const double distance_squared = dx * dx + dy * dy + dz * dz;
            const double distance = sqrt(distance_squared);
            const double cell_width = 2.0 * cell.half_extent;

            if (!contains_target && distance > 0.0 && cell_width / distance < opening_angle) {
                const double softened_distance = distance_squared + softening_squared;

                if (!isfinite(distance_squared) || !isfinite(softened_distance)) {
                    local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                    break;
                }
                if (softened_distance == 0.0) {
                    local_error = BLITZAR_STATUS_SINGULARITY;

                    break;
                }

                const double factor = gravitational_constant * cell.mass /
                                      (softened_distance * sqrt(softened_distance));

                if (!isfinite(factor)) {
                    local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                    break;
                }

                acceleration_x += factor * dx;
                acceleration_y += factor * dy;
                acceleration_z += factor * dz;

                continue;
            }

            for (int child = 7; child >= 0; --child) {
                const std::uint64_t child_index = cell.children[child];

                if (child_index == InvalidCell) {
                    continue;
                }
                if (stack_size >= MaxTraversalStack) {
                    local_error = BLITZAR_STATUS_INTERNAL_ERROR;

                    break;
                }

                traversal_stack[stack_size++] = child_index;
            }
        }
    }

    if (target < target_count &&
        (!isfinite(acceleration_x) || !isfinite(acceleration_y) || !isfinite(acceleration_z))) {
        local_error = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const int warp_error = BroadcastWarpError(local_error);

    if (warp_error > local_error) {
        local_error = warp_error;
    }
    if (local_error != 0) {
        RecordError(&block_error, local_error);
    }

    __syncthreads();

    if (threadIdx.x == 0 && block_error != 0) {
        RecordError(error, block_error);
    }
    if (target < target_count && local_error == 0) {
        force_x[target] = acceleration_x;
        force_y[target] = acceleration_y;
        force_z[target] = acceleration_z;
    }
}

} // namespace

blitzar_status LaunchBarnesHut(const BarnesHutLaunchRequest& request) noexcept
{
    const DeviceParticleAddresses& addresses = request.addresses;

    const std::size_t target_count = request.target_count;
    const std::size_t source_count = request.source_count;
    const double opening_angle = request.opening_angle;

    const TreeAddresses& tree = request.tree;
    const KernelPhysics& physics = request.physics;

    const std::size_t max_depth = request.max_depth;

    const KernelRuntime& runtime = request.runtime;

    if (target_count == 0) {
        return BLITZAR_STATUS_OK;
    }
    if (tree.cells == 0 || tree.indices == 0 || tree.cell_count == 0 ||
        runtime.error_address == 0 || runtime.stream == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (addresses.position_x == 0 || addresses.position_y == 0 || addresses.position_z == 0 ||
        addresses.mass == 0 || addresses.force_x == 0 || addresses.force_y == 0 ||
        addresses.force_z == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (max_depth > 36) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
    if (target_count > std::numeric_limits<std::size_t>::max() - (BlockSize - 1)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t block_count = (target_count + BlockSize - 1) / BlockSize;

    if (block_count > std::numeric_limits<unsigned int>::max()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const BarnesHutDeviceRequest kernel_request{
        reinterpret_cast<const double*>(addresses.position_x),
        reinterpret_cast<const double*>(addresses.position_y),
        reinterpret_cast<const double*>(addresses.position_z),
        reinterpret_cast<const double*>(addresses.mass),
        reinterpret_cast<double*>(addresses.force_x), reinterpret_cast<double*>(addresses.force_y),
        reinterpret_cast<double*>(addresses.force_z), target_count, source_count,
        reinterpret_cast<const GpuCell*>(tree.cells), tree.cell_count,
        reinterpret_cast<const std::uint64_t*>(tree.indices), opening_angle,
        physics.gravitational_constant, physics.softening,
        reinterpret_cast<int*>(runtime.error_address)};

    hipLaunchKernelGGL(BarnesHutKernel, dim3(static_cast<unsigned int>(block_count)),
        dim3(BlockSize), 0, reinterpret_cast<hipStream_t>(runtime.stream), kernel_request);

    return hipGetLastError() == hipSuccess ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_gpu_detail
