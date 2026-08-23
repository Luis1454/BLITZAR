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

constexpr unsigned int BlockSize = 256;

struct DirectDeviceRequest final {
    const double* position_x;
    const double* position_y;
    const double* position_z;
    const double* mass;
    double* force_x;
    double* force_y;
    double* force_z;
    std::size_t target_count;
    std::size_t source_begin;
    std::size_t source_end;
    double gravitational_constant;
    double softening;
    bool accumulate;
    int* error;
};

__device__ void RecordError(int* error, int status) noexcept
{
    atomicMax(error, status);
}

__global__ void DirectKernel(DirectDeviceRequest request)
{
    const double* position_x = request.position_x;
    const double* position_y = request.position_y;
    const double* position_z = request.position_z;
    const double* mass = request.mass;
    double* force_x = request.force_x;
    double* force_y = request.force_y;
    double* force_z = request.force_z;

    const std::size_t target_count = request.target_count;
    const std::size_t source_begin = request.source_begin;
    const std::size_t source_end = request.source_end;
    const double gravitational_constant = request.gravitational_constant;
    const double softening = request.softening;
    const bool accumulate = request.accumulate;

    int* error = request.error;

    extern __shared__ double tile[];

    double* tile_x = tile;
    double* tile_y = tile_x + BlockSize;
    double* tile_z = tile_y + BlockSize;
    double* tile_mass = tile_z + BlockSize;

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

    for (std::size_t tile_begin = source_begin; tile_begin < source_end; tile_begin += BlockSize) {
        const std::size_t source = tile_begin + threadIdx.x;

        if (source < source_end) {
            tile_x[threadIdx.x] = position_x[source];
            tile_y[threadIdx.x] = position_y[source];
            tile_z[threadIdx.x] = position_z[source];
            tile_mass[threadIdx.x] = mass[source];
        }
        else {
            tile_x[threadIdx.x] = 0.0;
            tile_y[threadIdx.x] = 0.0;
            tile_z[threadIdx.x] = 0.0;
            tile_mass[threadIdx.x] = 0.0;
        }

        __syncthreads();

        if (target < target_count) {
            const double target_x = position_x[target];
            const double target_y = position_y[target];
            const double target_z = position_z[target];
            const double target_mass = mass[target];

            if (!isfinite(target_x) || !isfinite(target_y) || !isfinite(target_z) ||
                !isfinite(target_mass) || target_mass < 0.0) {
                local_error = BLITZAR_STATUS_INVALID_ARGUMENT;
            }
#pragma unroll 4

            for (unsigned int offset = 0; offset < BlockSize; ++offset) {
                const std::size_t source_index = tile_begin + offset;

                if (source_index >= source_end) {
                    continue;
                }

                const double source_x = tile_x[offset];
                const double source_y = tile_y[offset];
                const double source_z = tile_z[offset];
                const double source_mass = tile_mass[offset];

                if (!isfinite(source_x) || !isfinite(source_y) || !isfinite(source_z) ||
                    !isfinite(source_mass) || source_mass < 0.0) {
                    local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                    continue;
                }
                if (source_index == target || source_mass == 0.0) {
                    continue;
                }

                const double dx = source_x - target_x;
                const double dy = source_y - target_y;
                const double dz = source_z - target_z;
                const double distance_squared = dx * dx + dy * dy + dz * dz;
                const double softened_distance = distance_squared + softening_squared;

                if (!isfinite(distance_squared) || !isfinite(softened_distance)) {
                    local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                    continue;
                }
                if (softened_distance == 0.0) {
                    local_error = BLITZAR_STATUS_SINGULARITY;

                    continue;
                }

                const double factor = gravitational_constant * source_mass /
                                      (softened_distance * sqrt(softened_distance));

                if (!isfinite(factor)) {
                    local_error = BLITZAR_STATUS_INVALID_ARGUMENT;

                    continue;
                }

                acceleration_x += factor * dx;
                acceleration_y += factor * dy;
                acceleration_z += factor * dz;
            }
        }

        __syncthreads();
    }

    if (target < target_count &&
        (!isfinite(acceleration_x) || !isfinite(acceleration_y) || !isfinite(acceleration_z))) {
        local_error = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (local_error != 0) {
        RecordError(&block_error, local_error);
    }

    __syncthreads();

    if (threadIdx.x == 0 && block_error != 0) {
        RecordError(error, block_error);
    }
    if (target < target_count && local_error == 0) {
        if (accumulate) {
            force_x[target] += acceleration_x;
            force_y[target] += acceleration_y;
            force_z[target] += acceleration_z;
        }
        else {
            force_x[target] = acceleration_x;
            force_y[target] = acceleration_y;
            force_z[target] = acceleration_z;
        }
    }
}

} // namespace

blitzar_status LaunchDirect(const DirectLaunchRequest& request) noexcept
{
    const DeviceParticleAddresses& addresses = request.addresses;

    const std::size_t target_count = request.target_count;
    const blitzar_core::ForceRange& range = request.range;

    const KernelPhysics& physics = request.physics;
    const KernelRuntime& runtime = request.runtime;

    if (target_count == 0 || runtime.error_address == 0 || runtime.stream == 0) {
        return target_count == 0 ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (range.source_begin > range.source_end) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (addresses.position_x == 0 || addresses.position_y == 0 || addresses.position_z == 0 ||
        addresses.mass == 0 || addresses.force_x == 0 || addresses.force_y == 0 ||
        addresses.force_z == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (target_count > std::numeric_limits<std::size_t>::max() - (BlockSize - 1)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t block_count = (target_count + BlockSize - 1) / BlockSize;

    if (block_count > std::numeric_limits<unsigned int>::max()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const DirectDeviceRequest kernel_request{
        reinterpret_cast<const double*>(addresses.position_x),
        reinterpret_cast<const double*>(addresses.position_y),
        reinterpret_cast<const double*>(addresses.position_z),
        reinterpret_cast<const double*>(addresses.mass),
        reinterpret_cast<double*>(addresses.force_x), reinterpret_cast<double*>(addresses.force_y),
        reinterpret_cast<double*>(addresses.force_z), target_count, range.source_begin,
        range.source_end, physics.gravitational_constant, physics.softening, range.accumulate,
        reinterpret_cast<int*>(runtime.error_address)};

    hipLaunchKernelGGL(DirectKernel, dim3(static_cast<unsigned int>(block_count)), dim3(BlockSize),
        4 * BlockSize * sizeof(double), reinterpret_cast<hipStream_t>(runtime.stream),
        kernel_request);

    return hipGetLastError() == hipSuccess ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_gpu_detail
