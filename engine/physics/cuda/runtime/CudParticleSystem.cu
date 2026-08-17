/*
 * @file engine/physics/cuda/runtime/CudParticleSystem.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 * @note Includes
 * adaptive cost guarding, selective TreePM references, preserved dyadic force timestamps,
 * and
 * the shared cosmological expansion runtime.
 */

#include "physics/octree/runtime/OctParticleSystemDeviceState.hpp"
#include "physics/cuda/buffer/CudMemoryPool.hpp"
#include "physics/fmm/model/FmmCpu.hpp"
#include "physics/treepm/cpu/TpmCpu.hpp"

// Fragment order encodes declaration dependencies; do not sort these includes.
// clang-format off
#include "physics/cuda/prelude/CudPrelude.inl"
#include "physics/octree/cuda/build/OctBuild.inl"
#include "physics/octree/cuda/force/OctForce.inl"
#include "physics/octree/cuda/morton/OctMortonSorting.inl"
#include "physics/octree/cuda/linear/OctLinearGpu.inl"
#include "physics/octree/cuda/runtime/OctGpu.inl"
#include "physics/treepm/cuda/math/TpmMath.inl"
#include "physics/treepm/cuda/fft/TpmFft.inl"
#include "physics/treepm/cuda/bounds/TpmBounds.inl"
#include "physics/treepm/cuda/deposit/TpmDeposit.inl"
#include "physics/treepm/cuda/neighbor/TpmNeighbor.inl"
#include "physics/treepm/cuda/local_grid/TpmLocalGrid.inl"
#include "physics/treepm/cuda/tree_force/TpmTreeForce.inl"
#include "physics/treepm/cuda/layout/TpmLayout.inl"
#include "physics/treepm/cuda/buffers/TpmBuffers.inl"
#include "physics/treepm/cuda/neighbor/TpmNeighborGrid.inl"
#include "physics/treepm/cuda/fft/TpmFftRuntime.inl"
#include "physics/treepm/cuda/graph/TpmGraph.inl"
#include "physics/treepm/cuda/cosmology/TpmCosmology.inl"
#include "physics/treepm/cuda/grid/TpmGridBuild.inl"
#include "physics/cuda/integration/CudAdaptive.inl"
#include "physics/sph/cuda/grid/SphGrid.inl"
#include "physics/sph/cuda/kernels/SphKernels.inl"
#include "physics/thermal/cuda/energy/ThmEnergyGpu.inl"
#include "physics/thermal/cuda/thermal/ThmThermal.inl"
#include "physics/cuda/integration/CudTiledAcceleration.inl"
#include "physics/cuda/integration/CudSphCorrection.inl"
#include "physics/cuda/integration/CudCpuAcceleration.inl"
#include "physics/cuda/integration/CudAdaptiveReference.inl"
#include "physics/cuda/integration/CudCpuUpdate.inl"
#include "physics/cuda/integration/CudOctreeGpuUpdate.inl"
#include "physics/cuda/integration/CudOctreeGpuRegular.inl"
#include "physics/cuda/integration/CudOctreeGpuAdaptive.inl"
#include "physics/cuda/integration/CudOctreeGpuIntegrators.inl"
#include "physics/cuda/integration/CudOctreeGpuFinalize.inl"
#include "physics/cuda/integration/CudPairwiseAcceleration.inl"
#include "physics/cuda/integration/CudAdaptiveUpdate.inl"
#include "physics/cuda/integration/CudIntegrators.inl"
#include "physics/cuda/integration/CudSolverUpdate.inl"
#include "physics/cuda/integration/CudProfilerUpdate.inl"
#include "physics/cuda/buffer/CudBuffer.inl"
#include "physics/cuda/core/CudState.inl"
#include "physics/cuda/core/CudCore.inl"
// clang-format on
// Keep this translation unit visible when shared system fragments change.

static_assert(alignof(GpuSystemMetrics) == 64, "GpuSystemMetrics alignment must remain 64");
static_assert(sizeof(GpuSystemMetrics) == 128,
              "GpuSystemMetrics layout must remain explicitly padded");
