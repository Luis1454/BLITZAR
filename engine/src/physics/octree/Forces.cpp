/*
 * @file engine/src/physics/octree/Forces.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU force dispatch for ParticleSystem solver strategies.
 */

#include "HostMath.hpp"
#include "OctreeForce.hpp"
#include "physics/core/ParticleSystem.hpp"
#include "physics/fmm/FmmCpu.hpp"
#include "physics/treepm/TreePmCpu.hpp"

#include <algorithm>
#include <cstddef>
#include <omp.h>

bool ParticleSystem::computeHostAccelerations(std::vector<Vector3>& accelerations)
{
    const std::size_t count = _particles.size();
    if (accelerations.size() != count) {
        accelerations.assign(count, Vector3());
    }
    const std::ptrdiff_t particleTotal = static_cast<std::ptrdiff_t>(count);
    std::vector<ParticleHotData> hotParticles;
    buildParticleHotData(_particles, hotParticles);
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);

    if (_solverMode == SolverMode::FmmCpu) {
        if (!_fmmWorkspace) {
            _fmmWorkspace = std::make_unique<bltzr_fmm::FmmWorkspace>();
        }
        bltzr_fmm::configure(*_fmmWorkspace, _fmmLeafCapacity, _octreeTheta);
        if (!bltzr_fmm::computeForces(_particles, forceLaw, *_fmmWorkspace, accelerations)) {
            return false;
        }
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t index = 0; index < particleTotal; ++index) {
            accelerations[static_cast<std::size_t>(index)] =
                blitzar_physics_particle_system_host::clampedVector(
                    accelerations[static_cast<std::size_t>(index)], _physicsMaxAcceleration);
        }
        return true;
    }

    const bool cpuFp64Reference =
        _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
    const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
    if (cpuFp64Reference) {
        if (!computeCpuFp64PairwiseForces(_particles, forceLaw, accelerations)) {
            return false;
        }
        if (!_device._treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=cpu_fp64_pairwise model=exact_tree precision=fp64\n");
            _device._treePmMarkerPrinted = true;
        }
    }
    else if (cpuTreePm) {
        CpuTreePmParameters parameters;
        parameters.model = _treePmModel;
        parameters.localGrid = _treePmLocalGrid;
        parameters.gridSize = _treePmGridSize;
        parameters.cutoffFactor = _treePmCutoffFactor;
        parameters.maxLocalNeighbors = _treePmMaxLocalNeighbors;
        parameters.particleLimit = _treePmParticleLimit;
        parameters.precision = _treePmPrecision;
        parameters.assignment = _treePmAssignment;
        bool computed = false;
        if (_treePmPrecision == "fp64") {
            if (!_cpuTreePmFp64Workspace) {
                _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
            }
            computed = computeCpuTreePmForcesFp64(_particles, forceLaw, parameters,
                                                  *_cpuTreePmFp64Workspace, _octree,
                                                  _octreeOpeningCriterion, accelerations);
        }
        else {
            if (!_cpuTreePmWorkspace) {
                _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
            }
            computed =
                computeCpuTreePmForces(_particles, forceLaw, parameters, *_cpuTreePmWorkspace,
                                       _octree, _octreeOpeningCriterion, accelerations);
        }
        if (!computed) {
            return false;
        }
        if (!_device._treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=cpu_fft_%s model=%s assignment=%s grid=%d "
                    "local_grid=%d neighbors=%d\n",
                    _treePmPrecision.c_str(), _treePmModel.c_str(), _treePmAssignment.c_str(),
                    std::clamp(_treePmGridSize, 32, 128), _treePmLocalGrid ? 1 : 0,
                    std::clamp(_treePmMaxLocalNeighbors, 0, 256));
            _device._treePmMarkerPrinted = true;
        }
    }
    else if (_solverMode == SolverMode::PairwiseCuda && count <= 4096u) {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t index = 0; index < particleTotal; ++index) {
            const std::size_t particleIndex = static_cast<std::size_t>(index);
            const Vector3 position = hotParticles[particleIndex].getPosition();
            Vector3 acceleration;
            for (std::size_t source = 0; source < count; ++source) {
                if (particleIndex != source) {
                    acceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                        position, hotParticles[source].getPosition(),
                        hotParticles[source].getMass(), forceLaw);
                }
            }
            accelerations[particleIndex] = acceleration;
        }
    }
    else {
        _octree.build(hotParticles);
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t index = 0; index < particleTotal; ++index) {
            const std::size_t particleIndex = static_cast<std::size_t>(index);
            accelerations[particleIndex] = _octree.computeForceOn(
                hotParticles, particleIndex, forceLaw, _octreeOpeningCriterion);
        }
    }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t index = 0; index < particleTotal; ++index) {
        accelerations[static_cast<std::size_t>(index)] =
            blitzar_physics_particle_system_host::clampedVector(
                accelerations[static_cast<std::size_t>(index)], _physicsMaxAcceleration);
    }
    return true;
}

bool ParticleSystem::computeHostAccelerationsForIndices(const std::vector<int>& activeIndices,
                                                        std::vector<Vector3>& accelerations)
{
    if (activeIndices.empty()) {
        return true;
    }
    const std::size_t count = _particles.size();
    if (accelerations.size() != count) {
        accelerations.assign(count, Vector3());
    }
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);
    if (_solverMode == SolverMode::FmmCpu) {
        if (!_fmmWorkspace) {
            _fmmWorkspace = std::make_unique<bltzr_fmm::FmmWorkspace>();
        }
        bltzr_fmm::configure(*_fmmWorkspace, _fmmLeafCapacity, _octreeTheta);
        std::vector<Vector3> allAccelerations;
        if (!bltzr_fmm::computeForces(_particles, forceLaw, *_fmmWorkspace, allAccelerations)) {
            return false;
        }
        for (const int activeIndex : activeIndices) {
            accelerations[static_cast<std::size_t>(activeIndex)] =
                blitzar_physics_particle_system_host::clampedVector(
                    allAccelerations[static_cast<std::size_t>(activeIndex)],
                    _physicsMaxAcceleration);
        }
        return true;
    }
    const bool cpuFp64Reference =
        _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
    const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
    if (cpuTreePm) {
        CpuTreePmParameters parameters;
        parameters.model = _treePmModel;
        parameters.localGrid = _treePmLocalGrid;
        parameters.gridSize = _treePmGridSize;
        parameters.cutoffFactor = _treePmCutoffFactor;
        parameters.maxLocalNeighbors = _treePmMaxLocalNeighbors;
        parameters.particleLimit = _treePmParticleLimit;
        parameters.precision = _treePmPrecision;
        parameters.assignment = _treePmAssignment;
        bool computed = false;
        if (_treePmPrecision == "fp64") {
            if (!_cpuTreePmFp64Workspace) {
                _cpuTreePmFp64Workspace = std::make_unique<CpuTreePmFp64Workspace>();
            }
            computed = computeCpuTreePmForcesSelectiveFp64(
                _particles, activeIndices, forceLaw, parameters, *_cpuTreePmFp64Workspace, _octree,
                _octreeOpeningCriterion, accelerations);
        }
        else {
            if (!_cpuTreePmWorkspace) {
                _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
            }
            computed = computeCpuTreePmForcesSelective(_particles, activeIndices, forceLaw,
                                                       parameters, *_cpuTreePmWorkspace, _octree,
                                                       _octreeOpeningCriterion, accelerations);
        }
        return computed;
    }

    if (cpuFp64Reference) {
        const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(activeIndices.size());
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t index = 0; index < total; ++index) {
            const std::size_t target =
                static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(index)]);
            Vector3 acceleration;
            const Vector3 position = _particles[target].getPosition();
            for (std::size_t source = 0; source < count; ++source) {
                if (source != target) {
                    acceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                        position, _particles[source].getPosition(), _particles[source].getMass(),
                        forceLaw);
                }
            }
            accelerations[target] = blitzar_physics_particle_system_host::clampedVector(
                acceleration, _physicsMaxAcceleration);
        }
        return true;
    }

    std::vector<ParticleHotData> hotParticles;
    buildParticleHotData(_particles, hotParticles);
    const std::ptrdiff_t total = static_cast<std::ptrdiff_t>(activeIndices.size());
    if (_solverMode == SolverMode::PairwiseCuda && count <= 4096u) {
#pragma omp parallel for schedule(static) if (!_deterministicMode)
        for (std::ptrdiff_t index = 0; index < total; ++index) {
            const std::size_t target =
                static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(index)]);
            const Vector3 position = hotParticles[target].getPosition();
            Vector3 acceleration;
            for (std::size_t source = 0; source < count; ++source) {
                if (source != target) {
                    acceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                        position, hotParticles[source].getPosition(),
                        hotParticles[source].getMass(), forceLaw);
                }
            }
            accelerations[target] = blitzar_physics_particle_system_host::clampedVector(
                acceleration, _physicsMaxAcceleration);
        }
        return true;
    }

#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t index = 0; index < total; ++index) {
        const std::size_t target =
            static_cast<std::size_t>(activeIndices[static_cast<std::size_t>(index)]);
        accelerations[target] = blitzar_physics_particle_system_host::clampedVector(
            _octree.computeForceOn(hotParticles, target, forceLaw, _octreeOpeningCriterion),
            _physicsMaxAcceleration);
    }
    return true;
}
