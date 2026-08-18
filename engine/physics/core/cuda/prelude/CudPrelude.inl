/*
 * @file engine/physics/core/cuda/prelude/CudPrelude.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for shared CUDA system helpers and kernels.
 */

/*
 * Fragment order is intentional: shared types and helpers are defined before
 * the kernels and transfer operations that consume them.
 */

#include "physics/core/cuda/prelude/CudConfiguration.inl"
#include "physics/core/cuda/prelude/CudForceKernels.inl"
#include "physics/core/cuda/prelude/CudMetricsKernels.inl"
#include "physics/core/cuda/prelude/CudIntegrationKernels.inl"
#include "physics/core/cuda/prelude/CudParticleKernels.inl"
#include "physics/core/cuda/prelude/CudCosmologyKernels.inl"
#include "physics/core/cuda/prelude/CudSoaKernels.inl"
