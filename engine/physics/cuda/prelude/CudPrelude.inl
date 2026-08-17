/*
 * @file engine/physics/cuda/prelude/CudPrelude.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for shared CUDA system helpers and kernels.
 */

/*
 * Fragment order is intentional: shared types and helpers are defined before
 * the kernels and transfer operations that consume them.
 */

#include "physics/cuda/prelude/CudConfiguration.inl"
#include "physics/cuda/prelude/CudForceKernels.inl"
#include "physics/cuda/prelude/CudMetricsKernels.inl"
#include "physics/cuda/prelude/CudIntegrationKernels.inl"
#include "physics/cuda/prelude/CudParticleKernels.inl"
#include "physics/cuda/prelude/CudCosmologyKernels.inl"
#include "physics/cuda/prelude/CudSoaKernels.inl"
