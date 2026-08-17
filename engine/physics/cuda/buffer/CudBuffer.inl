/*
 * @file engine/physics/cuda/buffer/CudBuffer.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system buffer lifecycle fragments.
 */

/*
 * Fragment order is intentional: the implementation is split by responsibility,
 * while the CUDA translation unit retains its original declaration order.
 */

#include "physics/cuda/buffer/CudPlanning.inl"
#include "physics/cuda/buffer/CudRuntimeState.inl"
#include "physics/cuda/buffer/CudParticleBuffers.inl"
#include "physics/cuda/buffer/CudAdaptiveScratch.inl"
#include "physics/cuda/buffer/CudRelease.inl"
#include "physics/cuda/buffer/CudMappedMetrics.inl"
#include "physics/cuda/buffer/CudOctreeScratch.inl"
#include "physics/cuda/buffer/CudEnergyScratch.inl"
#include "physics/cuda/buffer/CudIntegratorScratch.inl"
#include "physics/cuda/buffer/CudSphScratch.inl"
#include "buffer/CudDeviceState.inl"
