#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_

#include "physics/CudaMemoryPool.hpp"
#include "physics/ForceLawPolicy.hpp"
#include "physics/Octree.hpp"
#include "physics/ParticleSoAView.hpp"

#include <vector_types.h>

/*
 * Module: physics
 * Responsibility: Own the particle-state buffers and advance the gravitational/SPH simulation.
 */

#include <vector>
#include <cstdint>
#include <string>

struct alignas(64) GpuSystemMetrics {
    std::uint32_t sequence;
    std::uint32_t flags;
    std::uint64_t stepId;
    float simTime;
    float dt;
    std::uint32_t particleCount;
    std::uint32_t nanCount;
    std::uint32_t infCount;
    float minSpeed;
    float maxSpeed;
    float kineticEnergy;
    float potentialEnergy;
    float totalEnergy;
    std::uint64_t vramUsedBytes;
    std::uint64_t vramPeakBytes;
    std::uint32_t reserved0;
    std::uint32_t reservedAlignment;
    std::uint64_t reserved1;
    std::uint64_t reserved2;
    std::uint64_t reserved3;
    std::uint64_t reserved4;
    std::uint64_t reserved5;
    std::uint64_t reserved6;
};

struct GpuMetricsPayload {
    std::uint32_t flags;
    std::uint64_t stepId;
    float simTime;
    float dt;
    std::uint32_t particleCount;
    std::uint32_t nanCount;
    std::uint32_t infCount;
    float minSpeed;
    float maxSpeed;
    float kineticEnergy;
    float potentialEnergy;
    float totalEnergy;
    std::uint64_t vramUsedBytes;
    std::uint64_t vramPeakBytes;
};

enum GpuMetricsFlags : std::uint32_t {
    kGpuMetricsValid = 1u << 0,
    kGpuMetricsEstimated = 1u << 1
};

/// Owns particle state on host and device and advances it with the selected solver.
class ParticleSystem {
    public:
        enum class SolverMode {
            PairwiseCuda,
            OctreeCpu,
            OctreeGpu
        };
        enum class IntegratorMode {
            Euler,
            Rk4,
            Leapfrog
        };

        /// Builds a particle system with `numParticles`, optionally using the built-in bootstrap state.
        ParticleSystem(int numParticles, bool bootstrapInitialState = true);
        /// Builds a particle system from an explicit particle set.
        explicit ParticleSystem(std::vector<Particle> initialParticles);
        /// Releases all host and device allocations owned by the system.
        ~ParticleSystem();

        /// Advances the system by `deltaTime` seconds.
        bool update(float deltaTime);
        /// Switches between pairwise CUDA and octree-driven gravity.
        void setUseOctree(bool enabled);
        /// Reports whether an octree-backed gravity solver is active.
        bool usesOctree() const;
        /// Sets the Barnes-Hut opening angle threshold.
        void setOctreeTheta(float theta);
        /// Selects the Barnes-Hut opening criterion.
        void setOctreeOpeningCriterion(OctreeOpeningCriterion criterion);
        /// Sets the minimum softening radius used by gravity interactions.
        void setOctreeSoftening(float softening);
        /// Enables or disables SPH processing.
        void setSphEnabled(bool enabled);
        /// Reports whether SPH processing is active.
        bool isSphEnabled() const;
        /// Updates the SPH parameter set in SI units.
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
        /// Updates the gravity stability clamps applied during integration.
        void setPhysicsStabilityConstants(float maxAcceleration, float minSoftening, float minDistance2, float minTheta);
        /// Updates the SPH acceleration and speed caps.
        void setSphCaps(float maxAcceleration, float maxSpeed);
        /// Updates the thermal model parameters in SI units.
        void setThermalParameters(float ambientTemperature, float specificHeat, float heatingCoeff, float radiationCoeff);
        /// Returns the cumulative radiated energy emitted since initialization.
        float getCumulativeRadiatedEnergy() const;
        /// Returns the thermal specific heat used by the thermal model.
        float getThermalSpecificHeat() const;
        /// Selects the gravity solver implementation.
        void setSolverMode(SolverMode mode);
        /// Returns the active gravity solver implementation.
        SolverMode getSolverMode() const;
        /// Selects the numerical integrator implementation.
        void setIntegratorMode(IntegratorMode mode);
        /// Returns the active numerical integrator implementation.
        IntegratorMode getIntegratorMode() const;
        /// Uploads the current host-side state to the device buffers.
        void syncDeviceState();
        /// Synchronizes device-side state back to host memory.
        bool syncHostState();
        /// Computes energy metrics directly from device buffers without host particle copies.
        bool computeEnergyEstimateGpu(
            std::size_t sampleLimit,
            float softening,
            float minDistance2,
            float specificHeat,
            float &kinetic,
            float &potential,
            float &thermal,
            bool &estimated);

        /// Returns the authoritative host-side particle vector.
        const std::vector<Particle> &getParticles() const;
        /// Replaces the host-side particle vector when the size matches the device capacity.
        bool setParticles(std::vector<Particle> particles);

        ParticleSystem(const ParticleSystem &) = delete;
        ParticleSystem &operator=(const ParticleSystem &) = delete;
        ParticleSystem(ParticleSystem &&) = delete;
        ParticleSystem &operator=(ParticleSystem &&) = delete;

        /// Exposes the current structure-of-arrays views for diagnostics and tests.
        ParticleSoAView getSoAView(bool next = false) const;
        /// Returns the mapped host metrics snapshot used for asynchronous telemetry reads.
        const GpuSystemMetrics *getMappedGpuMetrics() const;

    private:
        void initializeRuntimeState(std::size_t particleCapacity);
        void buildBootstrapState(int particleCount);
        bool allocateParticleBuffers(std::size_t particleCapacity);
        bool seedDeviceState();
        void releaseParticleBuffers();
        float applyThermalModel(float deltaTime);
        bool buildSphGrid(int numParticles);
        void releaseRk4Buffers();
        void releaseSphBuffers();
        void releaseSphGridBuffers();
        bool allocateRk4Buffers(int numParticles);
        bool allocateSphBuffers(int numParticles);
        bool allocateSphGridBuffers(int numParticles);
        bool ensureLinearOctreeScratchCapacity(int numParticles);
        bool ensureEnergyScratchCapacity(int numParticles, int sampleCount);
        bool buildLinearOctreeGpu(ParticleSoAView currentView, int numParticles);
        bool allocateMappedMetrics();
        void releaseMappedMetrics();
        void publishMappedMetrics(float deltaTime);
        std::size_t estimateMemoryUsage(
            std::size_t particleCount,
            bool sphEnabled,
            SolverMode solverMode,
            IntegratorMode integratorMode,
            std::size_t energySampleLimit,
            int octreeLeafCapacity,
            std::size_t *baseAndIntegratorBytes,
            std::size_t *sphBytes,
            std::size_t *octreeBytes) const;
        static std::string formatMemoryBreakdown(
            std::size_t baseAndIntegratorBytes,
            std::size_t sphBytes,
            std::size_t octreeBytes,
            std::size_t totalBytes,
            std::size_t budgetBytes);

        std::vector<Particle> _particles;
        SolverMode _solverMode;
        IntegratorMode _integratorMode;
        float _octreeTheta;
        OctreeOpeningCriterion _octreeOpeningCriterion;
        float _octreeSoftening;
        bool _sphEnabled;
        float _sphSmoothingLength;
        float _sphRestDensity;
        float _sphGasConstant;
        float _sphViscosity;
        float _physicsMaxAcceleration;
        float _physicsMinSoftening;
        float _physicsMinDistance2;
        float _physicsMinTheta;
        float _sphMaxAcceleration;
        float _sphMaxSpeed;
        float _thermalAmbientTemperature;
        float _thermalSpecificHeat;
        float _thermalHeatingCoeff;
        float _thermalRadiationCoeff;
        float _cumulativeRadiatedEnergy;
        Octree _octree;
        std::vector<Vector3> _octreeForces;
        std::vector<GpuOctreeNode> _octreeGpuNodes;
        std::vector<int> _octreeGpuLeafIndices;
        std::size_t _deviceParticleCapacity;
        bool _cudaRuntimeAvailable;
        bool _hostStateDirty;
        int _sphGridSize;
        int _sphGridTotalCells;

        // Device buffers (SoA layout)
        float *d_soaPosX;
        float *d_soaPosY;
        float *d_soaPosZ;
        float *d_soaVelX;
        float *d_soaVelY;
        float *d_soaVelZ;
        float *d_soaPressX;
        float *d_soaPressY;
        float *d_soaPressZ;
        float *d_soaMass;
        float *d_soaTemp;
        float *d_soaDens;

        // Double buffering for update
        float *d_soaNextPosX;
        float *d_soaNextPosY;
        float *d_soaNextPosZ;
        float *d_soaNextVelX;
        float *d_soaNextVelY;
        float *d_soaNextVelZ;

        // Stage buffer for RK4 or thermal sync
        Particle *d_stage;

        Vector3 *d_k1x;
        Vector3 *d_k2x;
        Vector3 *d_k3x;
        Vector3 *d_k4x;
        Vector3 *d_k1v;
        Vector3 *d_k2v;
        Vector3 *d_k3v;
        Vector3 *d_k4v;
        float3 *d_vHalf;
        bool _leapfrogPrimed;
        float *d_sphDensity;
        float *d_sphPressure;
        int *d_sphCellHash;
        int *d_sphSortedIndex;
        int *d_sphCellStart;
        int *d_sphCellEnd;
        GpuOctreeNode *g_dOctreeNodes;
        int *g_dOctreeLeafIndices;
        unsigned long long *d_octreeMortonKeys;
        unsigned long long *d_octreePrefixesA;
        unsigned long long *d_octreePrefixesB;
        int *d_octreeLevelIndicesA;
        int *d_octreeLevelIndicesB;
        int *d_octreeParentCounts;
        int *d_octreeParentOffsets;
        GpuOctreeNodeHotData *d_octreeNodeHot;
        GpuOctreeNodeNavData *d_octreeNodeNav;
        int *d_octreeFirstChild;
        int *d_octreeLeafStarts;
        int *d_octreeLeafCounts;
        float *d_energyKineticBlocks;
        float *d_energyThermalBlocks;
        double *d_energyPotentialPartials;

        // Host shadows for SPH
        std::vector<int> _hostCellHash;
        std::vector<int> _hostSortedIndex;

        // Octree capacity tracking
        std::size_t g_dOctreeNodeCapacity;
        std::size_t g_dOctreeLeafCapacity;
        std::size_t d_octreeMortonCapacity;
        std::size_t d_octreePrefixCapacity;
        std::size_t d_octreeLevelCapacity;
        std::size_t d_energyBlockCapacity;
        std::size_t d_energySampleCapacity;
        int _gpuOctreeRootIndex;
        int _gpuOctreeNodeCount;
        int _gpuOctreeLeafCount;
        GpuSystemMetrics *_mappedMetricsHost;
        GpuSystemMetrics *_mappedMetricsDevice;
        std::uint64_t _metricsStepId;
        float _metricsSimTime;
        int _linearOctreeLeafCapacity;
};




#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_
