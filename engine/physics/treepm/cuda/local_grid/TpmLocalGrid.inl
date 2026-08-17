/*
 * @file engine/physics/treepm/cuda/local_grid/TpmLocalGrid.inl
 * @project BLITZAR
 * @brief TreePM local-grid force evaluation.
 */

namespace blitzar_cuda_tree_pm_gpu {

__device__ __forceinline__ Vector3 treePmComputeLocalGridAcceleration(
    ParticleSoAView state, int selfIndex, TreePmGridParams grid, IndexConstHandle sortedIndex,
    IndexConstHandle cellStart, IndexConstHandle cellEnd, ForceLawPolicy forceLaw,
    float maxAcceleration, const float* accelX, const float* accelY, const float* accelZ,
    const unsigned int* cellMask, float cutoffSquared, int cellRadius, int maxLocalNeighbors,
    const float* sortedPosX, const float* sortedPosY, const float* sortedPosZ,
    const float* sortedMass)
{
    constexpr int kMaxTreePmCellRadius = 2;
    const Vector3 selfPos = octreeLoadParticlePosition(state, selfIndex);
    Vector3 force = treePmSampleAcceleration(grid, selfPos, accelX, accelY, accelZ);
    if (maxLocalNeighbors <= 0) {
        return clampAcceleration(force, maxAcceleration);
    }

    int acceptedNeighbors = 0;
    int examinedCandidates = 0;
    const int maxExaminedCandidates = max(maxLocalNeighbors * 4, maxLocalNeighbors);

    const int centerX =
        min(max(static_cast<int>(floorf((selfPos.x - grid.originX) * grid.invCellSize)), 0),
            grid.gridSize - 1);
    const int centerY =
        min(max(static_cast<int>(floorf((selfPos.y - grid.originY) * grid.invCellSize)), 0),
            grid.gridSize - 1);
    const int centerZ =
        min(max(static_cast<int>(floorf((selfPos.z - grid.originZ) * grid.invCellSize)), 0),
            grid.gridSize - 1);
    const int boundedRadius = min(max(cellRadius, 1), kMaxTreePmCellRadius);

    for (int shell = 0; shell <= kMaxTreePmCellRadius; ++shell) {
        if (shell > boundedRadius || acceptedNeighbors >= maxLocalNeighbors ||
            examinedCandidates >= maxExaminedCandidates) {
            break;
        }
        for (int dz = -kMaxTreePmCellRadius; dz <= kMaxTreePmCellRadius; ++dz) {
            if (acceptedNeighbors >= maxLocalNeighbors ||
                examinedCandidates >= maxExaminedCandidates) {
                break;
            }
            if (abs(dz) > shell) {
                continue;
            }
            const int z = centerZ + dz;
            if (z < 0 || z >= grid.gridSize) {
                continue;
            }
            for (int dy = -kMaxTreePmCellRadius; dy <= kMaxTreePmCellRadius; ++dy) {
                if (acceptedNeighbors >= maxLocalNeighbors ||
                    examinedCandidates >= maxExaminedCandidates) {
                    break;
                }
                if (abs(dy) > shell) {
                    continue;
                }
                const int y = centerY + dy;
                if (y < 0 || y >= grid.gridSize) {
                    continue;
                }
                for (int dx = -kMaxTreePmCellRadius; dx <= kMaxTreePmCellRadius; ++dx) {
                    if (acceptedNeighbors >= maxLocalNeighbors ||
                        examinedCandidates >= maxExaminedCandidates) {
                        break;
                    }
                    if (abs(dx) > shell || max(max(abs(dx), abs(dy)), abs(dz)) != shell) {
                        continue;
                    }
                    const int x = centerX + dx;
                    if (x < 0 || x >= grid.gridSize) {
                        continue;
                    }
                    const int cell = treePmGridIndex(x, y, z, grid);
                    if (!treePmCellMaskEnabled(cellMask, cell)) {
                        continue;
                    }
                    const int begin = __ldg(&cellStart[cell]);
                    const int end = __ldg(&cellEnd[cell]);
                    if (begin < 0 || end <= begin) {
                        continue;
                    }
                    for (int cursor = begin; cursor < end; ++cursor) {
                        if (acceptedNeighbors >= maxLocalNeighbors ||
                            examinedCandidates >= maxExaminedCandidates) {
                            break;
                        }
                        const int otherIndex = __ldg(&sortedIndex[cursor]);
                        if (otherIndex == selfIndex) {
                            continue;
                        }
                        ++examinedCandidates;
                        const Vector3 sourcePos = sortedPosX != nullptr
                                                       ? Vector3(__ldg(&sortedPosX[cursor]),
                                                                 __ldg(&sortedPosY[cursor]),
                                                                 __ldg(&sortedPosZ[cursor]))
                                                       : octreeLoadParticlePosition(state, otherIndex);
                        const Vector3 diff(selfPos.x - sourcePos.x, selfPos.y - sourcePos.y,
                                           selfPos.z - sourcePos.z);
                        if (dot(diff, diff) > cutoffSquared) {
                            continue;
                        }
                        const float sourceMass = sortedMass != nullptr
                                                     ? __ldg(&sortedMass[cursor])
                                                     : octreeLoadParticleMass(state, otherIndex);
                        ForceLawPolicy shortRangeLaw = forceLaw;
                        shortRangeLaw.treePmShortRangeScale = grid.shortRangeScale;
                        force += blitzarAccelerationFromSource(selfPos, sourcePos, sourceMass,
                                                               shortRangeLaw);
                        ++acceptedNeighbors;
                    }
                }
            }
        }
    }
    return clampAcceleration(force, maxAcceleration);
}

__global__ void updateParticlesTreePmLocalGridKernel(
    ParticleSoAView lastState, ParticleSoAView particlesOut, int numParticles,
    TreePmGridParams grid, IndexConstHandle sortedIndex, IndexConstHandle cellStart,
    IndexConstHandle cellEnd, ForceLawPolicy forceLaw, float deltaTime, float maxAcceleration,
    const float* accelX, const float* accelY, const float* accelZ, const unsigned int* cellMask,
    float cutoffSquared, int cellRadius, int maxLocalNeighbors, const float* sortedPosX,
    const float* sortedPosY, const float* sortedPosZ, const float* sortedMass)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= numParticles) {
        return;
    }

    const Vector3 selfPos = octreeLoadParticlePosition(lastState, i);
    const Vector3 force = treePmComputeLocalGridAcceleration(
        lastState, i, grid, sortedIndex, cellStart, cellEnd, forceLaw, maxAcceleration, accelX,
        accelY, accelZ, cellMask, cutoffSquared, cellRadius, maxLocalNeighbors, sortedPosX,
        sortedPosY, sortedPosZ, sortedMass);
    const Vector3 vel = octreeLoadParticleVelocity(lastState, i);
    const Vector3 nextVel = vel + force * deltaTime;
    const Vector3 nextPos = selfPos + nextVel * deltaTime;

    setSoAPressure(particlesOut, i, force * 100.0f);
    setSoAVelocity(particlesOut, i, nextVel);
    setSoAPosition(particlesOut, i, nextPos);

    particlesOut.mass[i] = __ldg(&lastState.mass[i]);
    if (particlesOut.temp != nullptr && lastState.temp != nullptr) {
        particlesOut.temp[i] = __ldg(&lastState.temp[i]);
    }
    if (particlesOut.dens != nullptr && lastState.dens != nullptr) {
        particlesOut.dens[i] = __ldg(&lastState.dens[i]);
    }
}

} // namespace blitzar_cuda_tree_pm_gpu
