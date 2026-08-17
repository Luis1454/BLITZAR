/*
 * @file engine/physics/octree/src/ParticleSystemDeviceState.hpp
 * @project BLITZAR
 * @brief Private CUDA storage owned by ParticleSystem.
 */

#ifndef BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_PARTICLESYSTEMDEVICESTATE_HPP_
#define BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_PARTICLESYSTEMDEVICESTATE_HPP_

#include "ParticleSystem.hpp"
#include "CudaJit.hpp"
#include "DeviceMemory.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

struct GpuHalfVelocity {
    float x;
    float y;
    float z;
};

struct ParticleSystemDeviceState final {
    std::size_t _deviceParticleCapacity = 0u;
    bool _cudaRuntimeAvailable = false;
    bool _hostStateDirty = false;
    bool _treePmMarkerPrinted = false;
    bool _treePmLayoutModeInitialized = false;
    int _treePmLayoutMode = 0;
    bool _treePmAutoLayoutResolved = false;
    bool _treePmAutoGather = false;
    bool _treePmAutoMorton = false;
    float _treePmAutoR80Ratio = 1.0f;
    bool _treePmFftActive = false;
    bool _treePmGraphCaptured[2] = {false, false};
    bool _treePmGraphMarkerPrinted = false;
    int _treePmGraphSlot = 0;
    blitzar_cuda_memory::OpaqueHandle<blitzar_cuda_memory::releaseGraphExec> _treePmGraphExec[2];
    blitzar_cuda_memory::OpaqueHandle<blitzar_cuda_memory::releaseStream> _treePmGraphStream;
    bool _cudaJitMarkerPrinted = false;
    bool _cudaJitForceMarkerPrinted = false;
    double _cudaE2eTotalMs = 0.0;
    std::uint64_t _cudaE2eSamples = 0u;
    int _sphGridSize = 0;
    int _sphGridTotalCells = 0;
    int _treePmGridSize = 0;
    int _treePmTotalCells = 0;
    std::unique_ptr<CudaJitRuntime> _cudaJit;

    blitzar_cuda_memory::DeviceBuffer<float> d_soaPosX;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaPosY;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaPosZ;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaVelX;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaVelY;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaVelZ;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaPressX;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaPressY;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaPressZ;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaMass;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaTemp;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaDens;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaNextPosX;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaNextPosY;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaNextPosZ;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaNextVelX;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaNextVelY;
    blitzar_cuda_memory::DeviceBuffer<float> d_soaNextVelZ;
    blitzar_cuda_memory::DeviceBuffer<Particle> d_stage;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k1x;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k2x;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k3x;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k4x;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k1v;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k2v;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k3v;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_k4v;
    blitzar_cuda_memory::DeviceBuffer<GpuHalfVelocity> d_vHalf;
    bool _leapfrogPrimed = false;
    blitzar_cuda_memory::DeviceBuffer<float> d_sphDensity;
    blitzar_cuda_memory::DeviceBuffer<float> d_sphPressure;
    blitzar_cuda_memory::DeviceBuffer<int> d_sphCellHash;
    blitzar_cuda_memory::DeviceBuffer<int> d_sphSortedIndex;
    blitzar_cuda_memory::DeviceBuffer<int> d_sphCellStart;
    blitzar_cuda_memory::DeviceBuffer<int> d_sphCellEnd;
    blitzar_cuda_memory::DeviceBuffer<int> d_treePmSortKeys;
    blitzar_cuda_memory::DeviceBuffer<int> d_treePmSortIndices;
    blitzar_cuda_memory::DeviceBuffer<int> d_treePmSortedCellHash;
    blitzar_cuda_memory::DeviceBuffer<std::byte> d_treePmSortTempStorage;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmSortedPosX;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmSortedPosY;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmSortedPosZ;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmSortedMass;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmDensity;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmPotentialA;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmPotentialB;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmAccelX;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmAccelY;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmAccelZ;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmBoundsPartial;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmBounds;
    blitzar_cuda_memory::DeviceBuffer<float> d_treePmRadialMassHistogram;
    blitzar_cuda_memory::DeviceBuffer<Vector3> d_adaptiveAcceleration;
    blitzar_cuda_memory::DeviceBuffer<std::uint8_t> d_adaptiveLevels;
    blitzar_cuda_memory::DeviceBuffer<std::uint64_t> d_adaptiveLastForceTicks;
    blitzar_cuda_memory::DeviceBuffer<std::byte> d_treePmSpectrum;
    blitzar_cuda_memory::DeviceBuffer<std::byte> d_treePmSpectrumX;
    blitzar_cuda_memory::DeviceBuffer<std::byte> d_treePmSpectrumY;
    blitzar_cuda_memory::DeviceBuffer<std::byte> d_treePmSpectrumZ;
    blitzar_cuda_memory::DeviceBuffer<unsigned int> d_treePmCellMask;
    blitzar_cuda_memory::DeviceBuffer<GpuOctreeNode> g_dOctreeNodes;
    blitzar_cuda_memory::DeviceBuffer<int> g_dOctreeLeafIndices;
    blitzar_cuda_memory::DeviceBuffer<unsigned long long> d_octreeMortonKeys;
    blitzar_cuda_memory::DeviceBuffer<unsigned long long> d_octreePrefixesA;
    blitzar_cuda_memory::DeviceBuffer<unsigned long long> d_octreePrefixesB;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeLevelIndicesA;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeLevelIndicesB;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeParentCounts;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeParentOffsets;
    blitzar_cuda_memory::DeviceBuffer<GpuOctreeNodeHotData> d_octreeNodeHot;
    blitzar_cuda_memory::DeviceBuffer<GpuOctreeNodeNavData> d_octreeNodeNav;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeFirstChild;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeLeafStarts;
    blitzar_cuda_memory::DeviceBuffer<int> d_octreeLeafCounts;
    blitzar_cuda_memory::DeviceBuffer<float> d_energyKineticBlocks;
    blitzar_cuda_memory::DeviceBuffer<float> d_energyThermalBlocks;
    blitzar_cuda_memory::DeviceBuffer<double> d_energyPotentialPartials;

    std::size_t g_dOctreeNodeCapacity = 0u;
    std::size_t g_dOctreeLeafCapacity = 0u;
    std::size_t d_octreeMortonCapacity = 0u;
    std::size_t d_octreePrefixCapacity = 0u;
    std::size_t d_octreeLevelCapacity = 0u;
    std::size_t d_energyBlockCapacity = 0u;
    std::size_t d_energySampleCapacity = 0u;
    std::size_t d_treePmCapacity = 0u;
    std::size_t d_treePmSpectrumCapacity = 0u;
    std::size_t d_treePmMaskWordCapacity = 0u;
    std::size_t d_treePmBoundsBlockCapacity = 0u;
    std::size_t d_treePmNeighborParticleCapacity = 0u;
    std::size_t d_treePmNeighborCellCapacity = 0u;
    std::size_t d_treePmSortTempCapacity = 0u;
    std::size_t d_treePmSortedParticleCapacity = 0u;
    std::size_t d_adaptiveCapacity = 0u;
    int _gpuOctreeRootIndex = -1;
    blitzar_cuda_memory::FftPlanHandle _treePmFftPlan;
    blitzar_cuda_memory::FftPlanHandle _treePmFftInversePlan;
    int _treePmFftPlanGridSize = 0;
    int _gpuOctreeNodeCount = 0;
    int _gpuOctreeLeafCount = 0;
    blitzar_cuda_memory::MappedHostBuffer<GpuSystemMetrics> _mappedMetricsHost;
    std::uintptr_t _mappedMetricsDevice = 0u;
    std::uint64_t _metricsStepId = 0u;
    float _metricsSimTime = 0.0f;
    std::uint32_t _metricsPublishCounter = 0u;
    int _linearOctreeLeafCapacity = 0;
};

#endif // BLITZAR_ENGINE_SRC_PHYSICS_OCTREE_PARTICLESYSTEMDEVICESTATE_HPP_
