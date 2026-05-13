/*
 * @file engine/src/cuda/ParticleSystem.cu
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

#include "MemoryPool.hpp"
#include "fragments/system/Prelude.inl"
#include "fragments/octree/Impl.inl"
#include "fragments/octree/MortonSorting.inl"
#include "fragments/octree/LinearGpu.inl"
#include "fragments/octree/Gpu.inl"
#include "fragments/treepm/Gpu.inl"
#include "fragments/sph/Grid.inl"
#include "fragments/sph/Kernels.inl"
#include "fragments/thermal/EnergyGpu.inl"
#include "fragments/thermal/Thermal.inl"
#include "fragments/integration/TiledAcceleration.inl"
#include "fragments/integration/Update.inl"
#include "fragments/system/Buffer.inl"
#include "fragments/system/State.inl"
#include "fragments/system/Core.inl"

static_assert(alignof(GpuSystemMetrics) == 64, "GpuSystemMetrics alignment must remain 64");
static_assert(sizeof(GpuSystemMetrics) == 128,
              "GpuSystemMetrics layout must remain explicitly padded");
