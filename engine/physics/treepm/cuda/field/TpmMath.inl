/*
 * @file engine/physics/treepm/cuda/field/TpmMath.inl
 * @project BLITZAR
 * @brief Shared TreePM sampling and math device helpers.
 */

namespace blitzar_cuda_tree_pm_gpu {

bool checkTreePmFftStatus(cufftResult status, const char* operation)
{
    if (status == CUFFT_SUCCESS) {
        return true;
    }
    fprintf(stderr, "[treepm] cuFFT failure operation=%s status=%d\n", operation,
            static_cast<int>(status));
    return false;
}

__device__ __forceinline__ int treePmGridIndex(int x, int y, int z, const TreePmGridParams& grid)
{
    return (z * grid.gridSize + y) * grid.gridSize + x;
}

__device__ __forceinline__ int treePmWrapIndex(int value, int size)
{
    const int wrapped = value % size;
    return wrapped < 0 ? wrapped + size : wrapped;
}

__device__ __forceinline__ unsigned int treePmMortonPart1By2(unsigned int value)
{
    value &= 0xffu;
    value = (value | (value << 8u)) & 0x00ff00ffu;
    value = (value | (value << 4u)) & 0x0f0f0f0fu;
    value = (value | (value << 2u)) & 0x33333333u;
    value = (value | (value << 1u)) & 0x55555555u;
    return value;
}

__device__ __forceinline__ int treePmMortonIndex(int x, int y, int z)
{
    return static_cast<int>(treePmMortonPart1By2(static_cast<unsigned int>(x)) |
                            (treePmMortonPart1By2(static_cast<unsigned int>(y)) << 1u) |
                            (treePmMortonPart1By2(static_cast<unsigned int>(z)) << 2u));
}

__device__ __forceinline__ float treePmBoundaryPotential(int x, int y, int z,
                                                         const TreePmGridParams& grid)
{
    const float px = grid.originX + static_cast<float>(x) * grid.cellSize;
    const float py = grid.originY + static_cast<float>(y) * grid.cellSize;
    const float pz = grid.originZ + static_cast<float>(z) * grid.cellSize;
    const float dx = px - grid.boundaryCenterX;
    const float dy = py - grid.boundaryCenterY;
    const float dz = pz - grid.boundaryCenterZ;
    const float distance2 = dx * dx + dy * dy + dz * dz +
                            grid.boundarySoftening * grid.boundarySoftening;
    return -grid.boundaryMass * rsqrtf(fmaxf(distance2, 1.0e-12f));
}

__device__ __forceinline__ void treePmSetCellMask(unsigned int* cellMask, int cell)
{
    atomicOr(&cellMask[cell >> 5], 1u << (cell & 31));
}

__device__ __forceinline__ bool treePmCellMaskEnabled(const unsigned int* cellMask, int cell)
{
    return (cellMask[cell >> 5] & (1u << (cell & 31))) != 0u;
}

__device__ __forceinline__ float treePmAssignmentWeight(float distance, int assignment)
{
    const float absoluteDistance = fabsf(distance);
    if (assignment == 1) {
        if (absoluteDistance < 0.5f) {
            return 0.75f - absoluteDistance * absoluteDistance;
        }
        if (absoluteDistance < 1.5f) {
            const float tail = 1.5f - absoluteDistance;
            return 0.5f * tail * tail;
        }
        return 0.0f;
    }
    if (assignment == 2) {
        if (absoluteDistance < 1.0f) {
            return (4.0f - 6.0f * absoluteDistance * absoluteDistance +
                    3.0f * absoluteDistance * absoluteDistance * absoluteDistance) /
                   6.0f;
        }
        if (absoluteDistance < 2.0f) {
            const float tail = 2.0f - absoluteDistance;
            return tail * tail * tail / 6.0f;
        }
        return 0.0f;
    }
    return absoluteDistance < 1.0f ? 1.0f - absoluteDistance : 0.0f;
}

__device__ __forceinline__ float treePmAssignmentWindow(float value, int assignment)
{
    const float sincValue = fabsf(value) < 1.0e-5f ? 1.0f : sinf(value) / value;
    const int power = assignment == 1 ? 6 : assignment == 2 ? 8 : 4;
    float result = 1.0f;
    for (int exponent = 0; exponent < power; ++exponent) {
        result *= sincValue;
    }
    return result;
}

__device__ __forceinline__ float treePmSampleField(const float* field, const TreePmGridParams& grid,
                                                   const Vector3& pos)
{
    const float scaledX = (pos.x - grid.originX) * grid.invCellSize;
    const float scaledY = (pos.y - grid.originY) * grid.invCellSize;
    const float scaledZ = (pos.z - grid.originZ) * grid.invCellSize;

    const float extent = static_cast<float>(grid.gridSize);
    const float clampedX = grid.periodic ? scaledX - floorf(scaledX / extent) * extent
                                         : fminf(fmaxf(scaledX, 0.0f), extent - 1.0f);
    const float clampedY = grid.periodic ? scaledY - floorf(scaledY / extent) * extent
                                         : fminf(fmaxf(scaledY, 0.0f), extent - 1.0f);
    const float clampedZ = grid.periodic ? scaledZ - floorf(scaledZ / extent) * extent
                                         : fminf(fmaxf(scaledZ, 0.0f), extent - 1.0f);

    const int centerX = min(max(static_cast<int>(floorf(clampedX + 0.5f)), 0), grid.gridSize - 1);
    const int centerY = min(max(static_cast<int>(floorf(clampedY + 0.5f)), 0), grid.gridSize - 1);
    const int centerZ = min(max(static_cast<int>(floorf(clampedZ + 0.5f)), 0), grid.gridSize - 1);
    float result = 0.0f;
    for (int dz = -2; dz <= 2; ++dz) {
        const int z = grid.periodic ? treePmWrapIndex(centerZ + dz, grid.gridSize)
                                    : min(max(centerZ + dz, 0), grid.gridSize - 1);
        const float wz = treePmAssignmentWeight(clampedZ - static_cast<float>(centerZ + dz),
                                                 grid.assignment);
        for (int dy = -2; dy <= 2; ++dy) {
            const int y = grid.periodic ? treePmWrapIndex(centerY + dy, grid.gridSize)
                                        : min(max(centerY + dy, 0), grid.gridSize - 1);
            const float wy = treePmAssignmentWeight(clampedY - static_cast<float>(centerY + dy),
                                                     grid.assignment);
            for (int dx = -2; dx <= 2; ++dx) {
                const int x = grid.periodic ? treePmWrapIndex(centerX + dx, grid.gridSize)
                                            : min(max(centerX + dx, 0), grid.gridSize - 1);
                const float wx = treePmAssignmentWeight(
                    clampedX - static_cast<float>(centerX + dx), grid.assignment);
                result += field[treePmGridIndex(x, y, z, grid)] * wx * wy * wz;
            }
        }
    }
    return result;
}

__device__ __forceinline__ Vector3 treePmSampleAcceleration(const TreePmGridParams& grid,
                                                            const Vector3& pos, const float* accelX,
                                                            const float* accelY,
                                                            const float* accelZ)
{
    return Vector3(treePmSampleField(accelX, grid, pos), treePmSampleField(accelY, grid, pos),
                   treePmSampleField(accelZ, grid, pos));
}

__device__ __forceinline__ float treePmModifiedBesselK1(float value)
{
    const float x = fmaxf(value, 1.0e-4f);
    if (x <= 2.0f) {
        const float y = x * x * 0.25f;
        const float i1 = (x * 0.5f) *
                         (1.0f + y * (0.87890594f + y * (0.51498869f +
                         y * (0.15084934f + y * (0.02658733f +
                         y * (0.00301532f + y * 0.00032411f))))));
        return logf(x * 0.5f) * i1 +
               (1.0f / x) * (1.0f + y * (0.15443144f + y * (-0.67278579f +
               y * (-0.18156897f + y * (-0.01919402f + y * (-0.00110404f +
               y * -0.00004686f))))));
    }
    const float y = 2.0f / x;
    return expf(-x) * rsqrtf(x) * 1.25331414f *
           (1.0f + y * (0.23498619f + y * (-0.03655620f +
           y * (0.01504268f + y * (-0.00780353f + y *
           (0.00325614f + y * -0.00068245f))))));
}

__device__ __forceinline__ float treePmSinc(float value)
{
    return fabsf(value) < 1.0e-5f ? 1.0f : sinf(value) / value;
}

} // namespace blitzar_cuda_tree_pm_gpu
