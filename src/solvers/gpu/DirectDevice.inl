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
    const double* source_position_x;
    const double* source_position_y;
    const double* source_position_z;
    const double* source_mass;
    std::size_t target_count;
    std::size_t source_begin;
    std::size_t source_end;
    std::size_t source_global_begin;
    double gravitational_constant;
    double softening;
    bool accumulate;
    int* error;
};

struct DirectTarget final {
    bool valid{};
    std::size_t index{};
    double position_x{};
    double position_y{};
    double position_z{};
    int error{};
};

struct DirectTile final {
    double* position_x;
    double* position_y;
    double* position_z;
    double* mass;
};

struct DirectTileWork final {
    DirectDeviceRequest request;
    DirectTarget target;
    DirectTile tile;
    std::size_t tile_begin{};
};

struct DirectResult final {
    double acceleration_x{};
    double acceleration_y{};
    double acceleration_z{};
    int error{};
};

__device__ DirectTarget LoadTarget(
    const DirectDeviceRequest& request, std::size_t target_index) noexcept
{
    if (target_index >= request.target_count) {
        return {};
    }

    DirectTarget target{true, target_index, request.position_x[target_index],
        request.position_y[target_index], request.position_z[target_index], 0};

    const double target_mass = request.mass[target_index];

    if (!isfinite(target.position_x) || !isfinite(target.position_y) ||
        !isfinite(target.position_z) || !isfinite(target_mass) || target_mass < 0.0) {
        target.valid = false;
        target.error = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return target;
}

__device__ void LoadTile(
    const DirectDeviceRequest& request, DirectTile tile, std::size_t tile_begin) noexcept
{
    const std::size_t source = tile_begin + threadIdx.x;

    if (source < request.source_end) {
        tile.position_x[threadIdx.x] = request.source_position_x[source];
        tile.position_y[threadIdx.x] = request.source_position_y[source];
        tile.position_z[threadIdx.x] = request.source_position_z[source];
        tile.mass[threadIdx.x] = request.source_mass[source];
    }
    else {
        tile.position_x[threadIdx.x] = 0.0;
        tile.position_y[threadIdx.x] = 0.0;
        tile.position_z[threadIdx.x] = 0.0;
        tile.mass[threadIdx.x] = 0.0;
    }

    __syncthreads();
}

__device__ bool IsFiniteSource(
    double source_x, double source_y, double source_z, double source_mass) noexcept
{
    return isfinite(source_x) && isfinite(source_y) && isfinite(source_z) &&
           isfinite(source_mass) && source_mass >= 0.0;
}

__device__ int AccumulateSource(
    const DirectTileWork& work, unsigned int offset, DirectResult& result) noexcept
{
    const double source_x = work.tile.position_x[offset];
    const double source_y = work.tile.position_y[offset];
    const double source_z = work.tile.position_z[offset];
    const double source_mass = work.tile.mass[offset];

    if (!IsFiniteSource(source_x, source_y, source_z, source_mass)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (work.request.source_global_begin + work.tile_begin + offset == work.target.index ||
        source_mass == 0.0) {
        return BLITZAR_STATUS_OK;
    }

    const double dx = source_x - work.target.position_x;
    const double dy = source_y - work.target.position_y;
    const double dz = source_z - work.target.position_z;
    const double distance_squared = dx * dx + dy * dy + dz * dz;
    const double softened_distance =
        distance_squared + work.request.softening * work.request.softening;

    if (!isfinite(distance_squared) || !isfinite(softened_distance)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (softened_distance == 0.0) {
        return BLITZAR_STATUS_SINGULARITY;
    }

    const double factor = work.request.gravitational_constant * source_mass /
                          (softened_distance * sqrt(softened_distance));

    if (!isfinite(factor)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    result.acceleration_x += factor * dx;
    result.acceleration_y += factor * dy;
    result.acceleration_z += factor * dz;

    return BLITZAR_STATUS_OK;
}

__device__ void AccumulateTile(const DirectTileWork& work, DirectResult& result) noexcept
{
#pragma unroll 4
    for (unsigned int offset = 0; offset < BlockSize; ++offset) {
        const std::size_t source_index = work.tile_begin + offset;

        if (source_index >= work.request.source_end) {
            continue;
        }

        const int status = AccumulateSource(work, offset, result);

        if (status != BLITZAR_STATUS_OK) {
            result.error = status;
        }
    }
}

__device__ void PublishResult(const DirectDeviceRequest& request, const DirectTarget& target,
    DirectResult& result, int* block_error) noexcept
{
    if (target.valid && (!isfinite(result.acceleration_x) || !isfinite(result.acceleration_y) ||
                            !isfinite(result.acceleration_z))) {
        result.error = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (result.error != 0) {
        RecordError(block_error, result.error);
    }

    __syncthreads();

    if (threadIdx.x == 0 && *block_error != 0) {
        RecordError(request.error, *block_error);
    }
    if (target.valid && result.error == 0) {
        if (request.accumulate) {
            request.force_x[target.index] += result.acceleration_x;
            request.force_y[target.index] += result.acceleration_y;
            request.force_z[target.index] += result.acceleration_z;
        }
        else {
            request.force_x[target.index] = result.acceleration_x;
            request.force_y[target.index] = result.acceleration_y;
            request.force_z[target.index] = result.acceleration_z;
        }
    }
}

__global__ void DirectKernel(DirectDeviceRequest request)
{
    extern __shared__ double tile[];

    DirectTile shared_tile{tile, tile + BlockSize, tile + 2 * BlockSize, tile + 3 * BlockSize};

    __shared__ int block_error;

    if (threadIdx.x == 0) {
        block_error = 0;
    }

    __syncthreads();

    const std::size_t target = static_cast<std::size_t>(blockIdx.x) * BlockSize + threadIdx.x;
    const DirectTarget target_state = LoadTarget(request, target);
    DirectResult result{0.0, 0.0, 0.0, target_state.error};

    for (std::size_t tile_begin = request.source_begin; tile_begin < request.source_end;
        tile_begin += BlockSize) {
        LoadTile(request, shared_tile, tile_begin);

        if (target_state.valid) {
            AccumulateTile({request, target_state, shared_tile, tile_begin}, result);
        }

        __syncthreads();
    }

    PublishResult(request, target_state, result, &block_error);
}

} // namespace

} // namespace blitzar_gpu_detail
