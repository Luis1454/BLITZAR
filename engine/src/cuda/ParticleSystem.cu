/*
 * @file engine/src/cuda/ParticleSystem.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 * @note Includes
 * adaptive cost guarding, selective TreePM references, preserved dyadic force timestamps,
 * and
 * the shared cosmological expansion runtime.
 */

#include "MemoryPool.hpp"
#include "physics/fmm/FmmCpu.hpp"
#include "physics/treepm/TreePmCpu.hpp"

// Fragment order encodes declaration dependencies; do not sort these includes.
// clang-format off
#include "fragments/system/Prelude.inl"
#include "fragments/octree/Impl.inl"
#include "fragments/octree/MortonSorting.inl"
#include "fragments/octree/LinearGpu.inl"
#include "fragments/octree/Gpu.inl"
#include "fragments/treepm/Gpu.inl"
#include "fragments/integration/Adaptive.inl"
#include "fragments/sph/Grid.inl"
#include "fragments/sph/Kernels.inl"
#include "fragments/thermal/EnergyGpu.inl"
#include "fragments/thermal/Thermal.inl"
#include "fragments/integration/TiledAcceleration.inl"
#include "fragments/integration/Update.inl"
#include "fragments/system/Buffer.inl"
#include "fragments/system/State.inl"
#include "fragments/system/Core.inl"
// clang-format on
// Keep this translation unit visible when shared system fragments change.

static_assert(alignof(GpuSystemMetrics) == 64, "GpuSystemMetrics alignment must remain 64");
static_assert(sizeof(GpuSystemMetrics) == 128,
              "GpuSystemMetrics layout must remain explicitly padded");
