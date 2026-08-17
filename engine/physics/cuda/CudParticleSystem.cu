/*
 * @file engine/physics/cuda/CudParticleSystem.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 * @note Includes
 * adaptive cost guarding, selective TreePM references, preserved dyadic force timestamps,
 * and
 * the shared cosmological expansion runtime.
 */

#include "OctParticleSystemDeviceState.hpp"
#include "CudMemoryPool.hpp"
#include "physics/fmm/FmmCpu.hpp"
#include "TpmCpu.hpp"

// Fragment order encodes declaration dependencies; do not sort these includes.
// clang-format off
#include "fragments/system/CudPrelude.inl"
#include "../octree/cuda/fragments/OctBuild.inl"
#include "../octree/cuda/fragments/OctForce.inl"
#include "../octree/cuda/fragments/OctMortonSorting.inl"
#include "../octree/cuda/fragments/OctLinearGpu.inl"
#include "../octree/cuda/fragments/OctGpu.inl"
#include "../treepm/cuda/fragments/TpmMath.inl"
#include "../treepm/cuda/fragments/TpmFft.inl"
#include "../treepm/cuda/fragments/TpmBounds.inl"
#include "../treepm/cuda/fragments/TpmDeposit.inl"
#include "../treepm/cuda/fragments/TpmNeighbor.inl"
#include "../treepm/cuda/fragments/TpmLocalGrid.inl"
#include "../treepm/cuda/fragments/TpmTreeForce.inl"
#include "../treepm/cuda/fragments/TpmLayout.inl"
#include "../treepm/cuda/fragments/TpmBuffers.inl"
#include "../treepm/cuda/fragments/TpmNeighborGrid.inl"
#include "../treepm/cuda/fragments/TpmFftRuntime.inl"
#include "../treepm/cuda/fragments/TpmGraph.inl"
#include "../treepm/cuda/fragments/TpmCosmology.inl"
#include "../treepm/cuda/fragments/TpmGridBuild.inl"
#include "fragments/integration/CudAdaptive.inl"
#include "../sph/cuda/fragments/SphGrid.inl"
#include "../sph/cuda/fragments/SphKernels.inl"
#include "../thermal/cuda/fragments/ThmEnergyGpu.inl"
#include "../thermal/cuda/fragments/ThmThermal.inl"
#include "fragments/integration/CudTiledAcceleration.inl"
#include "fragments/integration/CudSphCorrection.inl"
#include "fragments/integration/CudCpuAcceleration.inl"
#include "fragments/integration/CudAdaptiveReference.inl"
#include "fragments/integration/CudCpuUpdate.inl"
#include "fragments/integration/CudOctreeGpuUpdate.inl"
#include "fragments/integration/CudOctreeGpuRegular.inl"
#include "fragments/integration/CudOctreeGpuAdaptive.inl"
#include "fragments/integration/CudOctreeGpuIntegrators.inl"
#include "fragments/integration/CudOctreeGpuFinalize.inl"
#include "fragments/integration/CudPairwiseAcceleration.inl"
#include "fragments/integration/CudAdaptiveUpdate.inl"
#include "fragments/integration/CudIntegrators.inl"
#include "fragments/integration/CudSolverUpdate.inl"
#include "fragments/integration/CudProfilerUpdate.inl"
#include "fragments/system/CudBuffer.inl"
#include "fragments/system/CudState.inl"
#include "fragments/system/CudCore.inl"
// clang-format on
// Keep this translation unit visible when shared system fragments change.

static_assert(alignof(GpuSystemMetrics) == 64, "GpuSystemMetrics alignment must remain 64");
static_assert(sizeof(GpuSystemMetrics) == 128,
              "GpuSystemMetrics layout must remain explicitly padded");
