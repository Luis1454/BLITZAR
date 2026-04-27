#include "fragments/OctreeImpl.inl"
#include "fragments/ParticleSystemBuffer.inl"
#include "fragments/ParticleSystemCore.inl"
#include "fragments/ParticleSystemEnergyGpu.inl"
#include "fragments/ParticleSystemLinearOctreeGpu.inl"
#include "fragments/ParticleSystemMortonSorting.inl"
#include "fragments/ParticleSystemOctreeGpu.inl"
#include "fragments/ParticleSystemPrelude.inl"
#include "fragments/ParticleSystemSphGrid.inl"
#include "fragments/ParticleSystemSphKernels.inl"
#include "fragments/ParticleSystemState.inl"
#include "fragments/ParticleSystemThermal.inl"
#include "fragments/ParticleSystemTiledAcceleration.inl"
#include "fragments/ParticleSystemUpdate.inl"

static_assert(alignof(GpuSystemMetrics) == 64, "GpuSystemMetrics alignment must remain 64");
static_assert(sizeof(GpuSystemMetrics) == 128,
              "GpuSystemMetrics layout must remain explicitly padded");
