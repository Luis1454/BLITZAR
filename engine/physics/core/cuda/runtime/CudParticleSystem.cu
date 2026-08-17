/*
 * @file engine/physics/core/cuda/runtime/CudParticleSystem.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 * @note Includes
 * adaptive cost guarding, selective TreePM references, preserved dyadic force timestamps,
 * and
 * the shared cosmological expansion runtime.
 */

#include "physics/octree/model/OctParticleSystemDeviceState.hpp"
#include "physics/core/cuda/buffer/CudMemoryPool.hpp"
#include "physics/fmm/model/FmmCpu.hpp"
#include "physics/treepm/model/TpmCpu.hpp"

// Fragment order encodes declaration dependencies; do not sort these includes.
// clang-format off
#include "physics/core/cuda/prelude/CudPrelude.inl"
#include "physics/octree/cuda/build/OctBuild.inl"
#include "physics/octree/cuda/force/OctForce.inl"
#include "physics/octree/cuda/morton/OctMortonSorting.inl"
#include "physics/octree/cuda/linear/OctLinearGpu.inl"
#include "physics/octree/cuda/force/OctGpu.inl"
#include "physics/treepm/cuda/field/TpmMath.inl"
#include "physics/treepm/cuda/fft/TpmFft.inl"
#include "physics/treepm/cuda/layout/TpmBounds.inl"
#include "physics/treepm/cuda/deposit/TpmDeposit.inl"
#include "physics/treepm/short_range/TpmNeighbor.inl"
#include "physics/treepm/short_range/TpmLocalGrid.inl"
#include "physics/treepm/short_range/TpmTreeForce.inl"
#include "physics/treepm/cuda/layout/TpmLayout.inl"
#include "physics/treepm/cuda/layout/TpmBuffers.inl"
#include "physics/treepm/short_range/TpmNeighborGrid.inl"
#include "physics/treepm/cuda/fft/TpmFftRuntime.inl"
#include "physics/treepm/cuda/graph/TpmGraph.inl"
#include "physics/treepm/cuda/field/TpmCosmology.inl"
#include "physics/treepm/cuda/deposit/TpmGridBuild.inl"
#include "physics/core/cuda/integration/CudAdaptive.inl"
#include "physics/sph/grid/SphGrid.inl"
#include "physics/sph/kernels/SphKernels.inl"
#include "physics/thermal/energy/ThmEnergyGpu.inl"
#include "physics/thermal/energy/ThmThermal.inl"
#include "physics/core/cuda/integration/CudTiledAcceleration.inl"
#include "physics/core/cuda/integration/CudSphCorrection.inl"
#include "physics/core/cuda/integration/CudCpuAcceleration.inl"
#include "physics/core/cuda/integration/CudAdaptiveReference.inl"
#include "physics/core/cuda/integration/CudCpuUpdate.inl"
#include "physics/core/cuda/integration/CudOctreeGpuUpdate.inl"
#include "physics/core/cuda/integration/CudOctreeGpuRegular.inl"
#include "physics/core/cuda/integration/CudOctreeGpuAdaptive.inl"
#include "physics/core/cuda/integration/CudOctreeGpuIntegrators.inl"
#include "physics/core/cuda/integration/CudOctreeGpuFinalize.inl"
#include "physics/core/cuda/integration/CudPairwiseAcceleration.inl"
#include "physics/core/cuda/integration/CudAdaptiveUpdate.inl"
#include "physics/core/cuda/integration/CudIntegrators.inl"
#include "physics/core/cuda/integration/CudSolverUpdate.inl"
#include "physics/core/cuda/integration/CudProfilerUpdate.inl"
#include "physics/core/cuda/buffer/CudBuffer.inl"
#include "physics/core/cuda/core/CudState.inl"
#include "physics/core/cuda/core/CudCore.inl"
// clang-format on
// Keep this translation unit visible when shared system fragments change.

static_assert(alignof(GpuSystemMetrics) == 64, "GpuSystemMetrics alignment must remain 64");
static_assert(sizeof(GpuSystemMetrics) == 128,
              "GpuSystemMetrics layout must remain explicitly padded");
