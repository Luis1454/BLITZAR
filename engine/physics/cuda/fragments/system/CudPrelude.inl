/*
 * @file engine/physics/cuda/fragments/system/CudPrelude.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for shared CUDA system helpers and kernels.
 */

/*
 * Fragment order is intentional: shared types and helpers are defined before
 * the kernels and transfer operations that consume them.
 */

#include "prelude/CudConfiguration.inl"
#include "prelude/CudForceKernels.inl"
#include "prelude/CudMetricsKernels.inl"
#include "prelude/CudIntegrationKernels.inl"
#include "prelude/CudParticleKernels.inl"
#include "prelude/CudCosmologyKernels.inl"
#include "prelude/CudSoaKernels.inl"
