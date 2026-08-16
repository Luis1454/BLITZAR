/*
 * @file engine/src/cuda/fragments/system/Prelude.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for shared CUDA system helpers and kernels.
 */

/*
 * Fragment order is intentional: shared types and helpers are defined before
 * the kernels and transfer operations that consume them.
 */

#include "prelude/Configuration.inl"
#include "prelude/ForceKernels.inl"
#include "prelude/MetricsKernels.inl"
#include "prelude/IntegrationKernels.inl"
#include "prelude/ParticleKernels.inl"
#include "prelude/CosmologyKernels.inl"
#include "prelude/SoaKernels.inl"
