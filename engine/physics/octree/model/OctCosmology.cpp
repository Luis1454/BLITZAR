/*
 * @file engine/physics/octree/model/OctCosmology.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Host-side cosmological integration for ParticleSystem.
 */

#include "physics/octree/math/OctHostMath.hpp"
#include "physics/octree/force/OctreeForce.hpp"
#include "physics/octree/runtime/OctParticleSystemDeviceState.hpp"
#include "physics/core/particle/PhyParticleSystem.hpp"
#include "physics/treepm/cpu/TpmCpu.hpp"

#include <algorithm>
#include <cstdio>

bool ParticleSystem::updateComovingCosmology(float deltaTime)
{
    if (_cosmology.geometry != "cube" || !_treePmEnabled || _treePmModel != "pm_only" ||
        _sphEnabled) {
        fprintf(
            stderr,
            "[cosmology] comoving rejected: require cube, TreePM pm_only, SPH off, adaptive off\n");
        return false;
    }
    const float a0 = std::max(_cosmologyScaleFactor, 1.0e-6f);
    const float a1 = blitzar_physics_particle_system_host::advanceCosmologyScaleFactor(
        _cosmology, a0, deltaTime);
    const float amid = 0.5f * (a0 + a1);
    const float firstKick = 0.5f * deltaTime;
    const float drift =
        blitzar_physics_particle_system_host::comovingDriftFactor(_cosmology, a0, a1);
    const float boxLength = 2.0f * _cosmology.boxHalfExtent;
    if (a1 <= a0 || drift <= 0.0f || boxLength <= 0.0f) {
        return false;
    }

    auto computePmField = [&](float scaleFactor, std::vector<Vector3>& accelerations) {
        CpuTreePmParameters parameters;
        parameters.model = "pm_only";
        parameters.gridSize = _treePmGridSize;
        parameters.assignment = "tsc";
        parameters.periodic = true;
        parameters.densityContrast = true;
        parameters.boxLength = boxLength;
        parameters.poissonCoefficient = 1.5f * _cosmology.hubbleH0 * _cosmology.hubbleH0 *
                                        _cosmology.omegaMatter / std::max(scaleFactor, 1.0e-6f);
        if (!_cpuTreePmWorkspace) {
            _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
        }
        return computeCpuTreePmForces(_particles, ForceLawPolicy{}, parameters,
                                      *_cpuTreePmWorkspace, _octree, _octreeOpeningCriterion,
                                      accelerations);
    };

    std::vector<Vector3> firstAcceleration(_particles.size(), Vector3());
    _cpuTreePmWorkspace.reset();
    if (!computePmField(amid, firstAcceleration)) {
        return false;
    }
    for (std::size_t index = 0; index < _particles.size(); ++index) {
        const Vector3 momentum =
            _particles[index].getVelocity() + firstAcceleration[index] * firstKick;
        const Vector3 position = _particles[index].getPosition() + momentum * drift;
        _particles[index].setVelocity(momentum);
        _particles[index].setPosition(Vector3(
            blitzar_physics_particle_system_host::wrapComovingCoordinate(position.x, boxLength),
            blitzar_physics_particle_system_host::wrapComovingCoordinate(position.y, boxLength),
            blitzar_physics_particle_system_host::wrapComovingCoordinate(position.z, boxLength)));
    }

    std::vector<Vector3> secondAcceleration(_particles.size(), Vector3());
    _cpuTreePmWorkspace.reset();
    if (!computePmField(a1, secondAcceleration)) {
        return false;
    }
    for (std::size_t index = 0; index < _particles.size(); ++index) {
        _particles[index].setVelocity(_particles[index].getVelocity() +
                                      secondAcceleration[index] * firstKick);
        _particles[index].setPressure(secondAcceleration[index] * 100.0f);
    }
    _cosmologyScaleFactor = a1;
    _cosmologyTime += deltaTime;
    _device->_hostStateDirty = false;
    if (!_cosmologyMarkerPrinted) {
        fprintf(stderr,
                "[cosmology] mode=comoving backend=cpu_pm assignment=tsc box=%.6g a0=%.6g\n",
                boxLength, a0);
        _cosmologyMarkerPrinted = true;
    }
    return true;
}
