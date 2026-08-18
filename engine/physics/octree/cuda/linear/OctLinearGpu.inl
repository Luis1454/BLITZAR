/*
 * @file engine/physics/octree/cuda/linear/OctLinearGpu.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for the GPU linear octree implementation.
 */

/*
 * Fragment order is intentional: allocator and node types precede kernels,
 * and the orchestration method is included last.
 */

#include "physics/octree/cuda/linear/OctAllocator.inl"
#include "physics/octree/cuda/linear/OctKeyKernels.inl"
#include "physics/octree/cuda/linear/OctLeafKernels.inl"
#include "physics/octree/cuda/linear/OctParentKernels.inl"
#include "physics/octree/cuda/linear/OctLinkKernels.inl"
#include "physics/octree/cuda/linear/OctBuild.inl"
