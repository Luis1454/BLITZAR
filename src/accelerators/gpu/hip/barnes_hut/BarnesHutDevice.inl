namespace blitzar_accelerator_launch {

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

struct BarnesTarget final {
    bool valid{};
    std::size_t index{};
    double position_x{};
    double position_y{};
    double position_z{};
    int error{};
};

struct BarnesResult final {
    double acceleration_x{};
    double acceleration_y{};
    double acceleration_z{};
    int error{};
};

struct BarnesTraversal final {
    std::uint64_t* stack;
    int size{};
};

struct BarnesCellWork final {
    const BarnesHutDeviceRequest* request;
    const BarnesTarget* target;
    const GpuCell* cell;
    BarnesTraversal* traversal;
};

#include "accelerators/gpu/hip/barnes_hut/BarnesHutCell.inl"
#include "accelerators/gpu/hip/barnes_hut/BarnesHutWarp.inl"

__device__ BarnesTarget LoadTarget(
    const BarnesHutDeviceRequest& request, std::size_t target_index) noexcept
{
    if (target_index >= request.target_count) {
        return {};
    }

    BarnesTarget target{true, target_index, request.position_x[target_index],
        request.position_y[target_index], request.position_z[target_index], 0};

    const double target_mass = request.mass[target_index];

    if (!isfinite(target.position_x) || !isfinite(target.position_y) ||
        !isfinite(target.position_z) || !isfinite(target_mass) || target_mass < 0.0) {
        target.valid = false;
        target.error = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return target;
}

__device__ bool IsFiniteSource(
    double source_x, double source_y, double source_z, double source_mass) noexcept
{
    return isfinite(source_x) && isfinite(source_y) && isfinite(source_z) &&
           isfinite(source_mass) && source_mass >= 0.0;
}

__device__ int AccumulateSource(const BarnesHutDeviceRequest& request, const BarnesTarget& target,
    std::uint64_t source, BarnesResult& result) noexcept
{
    if (source >= request.source_count) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const double source_mass = request.mass[source];
    const double source_x = request.position_x[source];
    const double source_y = request.position_y[source];
    const double source_z = request.position_z[source];

    if (!IsFiniteSource(source_x, source_y, source_z, source_mass)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (source == target.index || source_mass == 0.0) {
        return BLITZAR_STATUS_OK;
    }

    const double dx = source_x - target.position_x;
    const double dy = source_y - target.position_y;
    const double dz = source_z - target.position_z;
    const double distance_squared = dx * dx + dy * dy + dz * dz;
    const double softened_distance = distance_squared + request.softening * request.softening;

    if (!isfinite(distance_squared) || !isfinite(softened_distance)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (softened_distance == 0.0) {
        return BLITZAR_STATUS_SINGULARITY;
    }

    const double factor = request.gravitational_constant * source_mass /
                          (softened_distance * sqrt(softened_distance));

    if (!isfinite(factor)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    result.acceleration_x += factor * dx;
    result.acceleration_y += factor * dy;
    result.acceleration_z += factor * dz;

    return BLITZAR_STATUS_OK;
}

__device__ void AccumulateLeaf(const BarnesHutDeviceRequest& request, const BarnesTarget& target,
    const GpuCell& cell, BarnesResult& result) noexcept
{
    for (std::uint64_t offset = 0; offset < cell.count; ++offset) {
        const std::uint64_t source = request.indices[cell.begin + offset];
        const int status = AccumulateSource(request, target, source, result);

        if (status != BLITZAR_STATUS_OK) {
            result.error = status;

            break;
        }
    }
}

__device__ bool TryAccumulateCell(const BarnesCellWork& work, BarnesResult& result) noexcept
{
    const BarnesHutDeviceRequest& request = *work.request;
    const BarnesTarget& target = *work.target;
    const GpuCell& cell = *work.cell;

    const double dx = cell.center_of_mass_x - target.position_x;
    const double dy = cell.center_of_mass_y - target.position_y;
    const double dz = cell.center_of_mass_z - target.position_z;
    const double distance_squared = dx * dx + dy * dy + dz * dz;
    const double distance = sqrt(distance_squared);
    const double cell_width = 2.0 * cell.half_extent;

    if (Contains(cell, target.position_x, target.position_y, target.position_z) ||
        distance <= 0.0 || cell_width / distance >= request.opening_angle) {
        return false;
    }

    const double softened_distance = distance_squared + request.softening * request.softening;

    if (!isfinite(distance_squared) || !isfinite(softened_distance)) {
        result.error = BLITZAR_STATUS_INVALID_ARGUMENT;

        return true;
    }
    if (softened_distance == 0.0) {
        result.error = BLITZAR_STATUS_SINGULARITY;

        return true;
    }

    const double factor =
        request.gravitational_constant * cell.mass / (softened_distance * sqrt(softened_distance));

    if (!isfinite(factor)) {
        result.error = BLITZAR_STATUS_INVALID_ARGUMENT;

        return true;
    }

    result.acceleration_x += factor * dx;
    result.acceleration_y += factor * dy;
    result.acceleration_z += factor * dz;

    return true;
}

__device__ int PushChildren(const GpuCell& cell, BarnesTraversal& traversal) noexcept
{
    for (int child = 7; child >= 0; --child) {
        const std::uint64_t child_index = cell.children[child];

        if (child_index == InvalidCell) {
            continue;
        }
        if (traversal.size >= MaxTraversalStack) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        traversal.stack[traversal.size++] = child_index;
    }

    return BLITZAR_STATUS_OK;
}

__device__ void VisitCell(const BarnesCellWork& work, BarnesResult& result) noexcept
{
    const BarnesHutDeviceRequest& request = *work.request;
    const GpuCell& cell = *work.cell;

    const int cell_status = ValidateCell(request, cell);

    if (cell_status != BLITZAR_STATUS_OK) {
        result.error = cell_status;

        return;
    }
    if (cell.mass == 0.0) {
        return;
    }
    if (IsLeaf(cell)) {
        AccumulateLeaf(request, *work.target, cell, result);

        return;
    }
    if (TryAccumulateCell(work, result)) {
        return;
    }

    result.error = PushChildren(cell, *work.traversal);
}

__device__ void PublishResult(const BarnesHutDeviceRequest& request, const BarnesTarget& target,
    BarnesResult& result, int* block_error) noexcept
{
    if (target.valid && (!isfinite(result.acceleration_x) || !isfinite(result.acceleration_y) ||
                            !isfinite(result.acceleration_z))) {
        result.error = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const int warp_error = BroadcastWarpError(result.error);

    if (warp_error > result.error) {
        result.error = warp_error;
    }
    if (result.error != 0) {
        RecordError(block_error, result.error);
    }

    __syncthreads();

    if (threadIdx.x == 0 && *block_error != 0) {
        RecordError(request.error, *block_error);
    }
    if (target.valid && result.error == 0) {
        request.force_x[target.index] = result.acceleration_x;
        request.force_y[target.index] = result.acceleration_y;
        request.force_z[target.index] = result.acceleration_z;
    }
}

__global__ void BarnesHutKernel(BarnesHutDeviceRequest request)
{
    __shared__ int block_error;

    if (threadIdx.x == 0) {
        block_error = 0;
    }

    __syncthreads();

    const std::size_t target_index = static_cast<std::size_t>(blockIdx.x) * BlockSize + threadIdx.x;

    const BarnesTarget target = LoadTarget(request, target_index);
    BarnesResult result{0.0, 0.0, 0.0, target.error};
    std::uint64_t stack[MaxTraversalStack];
    BarnesTraversal traversal{stack, 0};

    if (target.valid) {
        traversal.stack[traversal.size++] = 0;
    }

    while (traversal.size > 0 && result.error == 0) {
        const std::uint64_t cell_index = traversal.stack[--traversal.size];

        if (cell_index >= request.cell_count) {
            result.error = BLITZAR_STATUS_INTERNAL_ERROR;

            break;
        }

        const BarnesCellWork work{&request, &target, &request.cells[cell_index], &traversal};

        VisitCell(work, result);
    }

    PublishResult(request, target, result, &block_error);
}

} // namespace

} // namespace blitzar_accelerator_launch
