/*
 * @file engine/src/cuda/fragments/system/Buffer.inl
 * @project BLITZAR
 * @brief Ordered inclusion manifest for particle-system buffer lifecycle fragments.
 */

/*
 * Fragment order is intentional: the implementation is split by responsibility,
 * while the CUDA translation unit retains its original declaration order.
 */

#include "buffer/Planning.inl"
#include "buffer/RuntimeState.inl"
#include "buffer/ParticleBuffers.inl"
#include "buffer/AdaptiveScratch.inl"
#include "buffer/Release.inl"
#include "buffer/MappedMetrics.inl"
#include "buffer/OctreeScratch.inl"
#include "buffer/EnergyScratch.inl"
#include "buffer/IntegratorScratch.inl"
#include "buffer/SphScratch.inl"
#include "buffer/DeviceState.inl"
