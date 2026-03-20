#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_

#include "physics/Octree.hpp"
#include "physics/CudaMemoryPool.hpp"
#include "physics/ParticleSoAView.hpp"

#include <vector>

class ParticleSystem {
    public:
        enum class SolverMode {
            PairwiseCuda,
            OctreeCpu,
            OctreeGpu
        };
        enum class IntegratorMode {
            Euler,
            Rk4
        };

        ParticleSystem(int numParticles, bool bootstrapInitialState = true);
        explicit ParticleSystem(std::vector<Particle> initialParticles);
        ~ParticleSystem();

        bool update(float deltaTime);
        void setUseOctree(bool enabled);
        bool usesOctree() const;
        void setOctreeTheta(float theta);
        void setOctreeSoftening(float softening);
        void setSphEnabled(bool enabled);
        bool isSphEnabled() const;
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
        void setPhysicsStabilityConstants(float maxAcceleration, float minSoftening, float minDistance2, float minTheta);
        void setSphCaps(float maxAcceleration, float maxSpeed);
        void setThermalParameters(float ambientTemperature, float specificHeat, float heatingCoeff, float radiationCoeff);
        float getCumulativeRadiatedEnergy() const;
        float getThermalSpecificHeat() const;
        void setSolverMode(SolverMode mode);
        SolverMode getSolverMode() const;
        void setIntegratorMode(IntegratorMode mode);
        IntegratorMode getIntegratorMode() const;
        void syncDeviceState();
        bool syncHostState();

        const std::vector<Particle> &getParticles() const;
        bool setParticles(std::vector<Particle> particles);

        ParticleSystem(const ParticleSystem &) = delete;
        ParticleSystem &operator=(const ParticleSystem &) = delete;
        ParticleSystem(ParticleSystem &&) = delete;
        ParticleSystem &operator=(ParticleSystem &&) = delete;

        ParticleSoAView getSoAView(bool next = false) const;

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

        std::vector<Particle> _particles;
        SolverMode _solverMode;
        IntegratorMode _integratorMode;
        float _octreeTheta;
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
        float *d_sphDensity;
        float *d_sphPressure;
        int *d_sphCellHash;
        int *d_sphSortedIndex;
        int *d_sphCellStart;
        int *d_sphCellEnd;
        GpuOctreeNode *g_dOctreeNodes;
        int *g_dOctreeLeafIndices;

        // Host shadows for SPH
        std::vector<int> _hostCellHash;
        std::vector<int> _hostSortedIndex;

        // Octree capacity tracking
        std::size_t g_dOctreeNodeCapacity;
        std::size_t g_dOctreeLeafCapacity;
};




#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_PARTICLESYSTEM_HPP_
