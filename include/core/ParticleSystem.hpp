/*
** EPITECH PROJECT, 2024
** rtxcpp
** File description:
** ParticleSystem
*/

#ifndef PARTICLESYSTEM_HPP_
#define PARTICLESYSTEM_HPP_

#include "core/Octree.hpp"

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

        ParticleSystem(int numParticles);
        ~ParticleSystem();

        void update(float deltaTime);
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

        GRAVITY_HD static Vector3 getForce(Particle *last, Particle *particle, int numParticles, int idx, float dt);
        std::vector<Particle> &getParticles();

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
        GpuOctreeNode *_dOctreeNodes;
        int *_dOctreeLeafIndices;
        std::size_t _dOctreeNodeCapacity;
        std::size_t _dOctreeLeafCapacity;
};

#endif /* !PARTICLESYSTEM_HPP_ */

