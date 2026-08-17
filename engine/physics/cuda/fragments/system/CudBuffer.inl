/*
 * @file engine/physics/cuda/fragments/system/CudBuffer.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system buffer lifecycle fragments.
 */

/*
 * Fragment order is intentional: the implementation is split by responsibility,
 * while the CUDA translation unit retains its original declaration order.
 */

#include "buffer/CudPlanning.inl"
#include "buffer/CudRuntimeState.inl"
#include "buffer/CudParticleBuffers.inl"
#include "buffer/CudAdaptiveScratch.inl"
#include "buffer/CudRelease.inl"
#include "buffer/CudMappedMetrics.inl"
#include "buffer/CudOctreeScratch.inl"
#include "buffer/CudEnergyScratch.inl"
#include "buffer/CudIntegratorScratch.inl"
#include "buffer/CudSphScratch.inl"
#include "buffer/CudDeviceState.inl"
