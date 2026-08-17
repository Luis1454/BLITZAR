/*
 * @file engine/physics/core/cuda/buffer/CudBuffer.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system buffer lifecycle fragments.
 */

/*
 * Fragment order is intentional: the implementation is split by responsibility,
 * while the CUDA translation unit retains its original declaration order.
 */

#include "physics/core/cuda/buffer/CudPlanning.inl"
#include "physics/core/cuda/buffer/CudRuntimeState.inl"
#include "physics/core/cuda/buffer/CudParticleBuffers.inl"
#include "physics/core/cuda/buffer/CudAdaptiveScratch.inl"
#include "physics/core/cuda/buffer/CudRelease.inl"
#include "physics/core/cuda/buffer/CudMappedMetrics.inl"
#include "physics/core/cuda/buffer/CudOctreeScratch.inl"
#include "physics/core/cuda/buffer/CudEnergyScratch.inl"
#include "physics/core/cuda/buffer/CudIntegratorScratch.inl"
#include "physics/core/cuda/buffer/CudSphScratch.inl"
#include "buffer/CudDeviceState.inl"
