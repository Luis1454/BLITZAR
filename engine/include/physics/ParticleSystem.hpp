#pragma once

#include "physics/Octree.hpp"

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
        ~ParticleSystem();

        bool update(float deltaTime);
        void setUseOctree(bool enabled);
        bool usesOctree() const;
        void setOctreeTheta(float theta);
        void setOctreeSoftening(float softening);
        void setSphEnabled(bool enabled);
        bool isSphEnabled() const;
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
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

    private:
        float applyThermalModel(float deltaTime);

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
};



