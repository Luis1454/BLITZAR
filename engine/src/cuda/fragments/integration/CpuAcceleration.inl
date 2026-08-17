/*
 * @file engine/src/cuda/fragments/integration/CpuAcceleration.inl
 * @project BLITZAR
 * @brief Host-side force dispatch used by CPU and reference integration paths.
 */

#include <algorithm>
#include <cstdio>

/*
 * @brief Computes host accelerations using the selected CPU force backend.
 * @param state Particle state used as the force source and target set.
 * @param forceLaw Resolved softening and distance policy.
 * @param output Destination acceleration vector.
 * @return True when the selected backend completes successfully.
 */
bool ParticleSystem::computeCpuAcceleration(const std::vector<Particle>& state,
                                            const ForceLawPolicy& forceLaw,
                                            std::vector<Vector3>& output)
{
    if (_solverMode == SolverMode::FmmCpu) {
        if (!_fmmWorkspace) {
            _fmmWorkspace = std::make_unique<bltzr_fmm::FmmWorkspace>();
        }
        bltzr_fmm::configure(*_fmmWorkspace, _fmmLeafCapacity, _octreeTheta);
        return bltzr_fmm::computeForces(state, forceLaw, *_fmmWorkspace, output);
    }

    const bool cpuFp64Reference =
        _treePmEnabled && _treePmModel == "exact_tree" && _treePmPrecision == "fp64";
    const bool cpuTreePm = _treePmEnabled && _treePmModel != "exact_tree";
    if (cpuFp64Reference) {
        if (!computeCpuFp64PairwiseForces(state, forceLaw, output)) {
            return false;
        }
        if (!_device->_treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=cpu_fp64_pairwise model=exact_tree precision=fp64\n");
            _device->_treePmMarkerPrinted = true;
        }
        return true;
    }

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
            computed = computeCpuTreePmForcesFp64(state, forceLaw, parameters,
                                                  *_cpuTreePmFp64Workspace, _octree,
                                                  _octreeOpeningCriterion, output);
        }
        else {
            if (!_cpuTreePmWorkspace) {
                _cpuTreePmWorkspace = std::make_unique<CpuTreePmWorkspace>();
            }
            computed = computeCpuTreePmForces(state, forceLaw, parameters, *_cpuTreePmWorkspace,
                                              _octree, _octreeOpeningCriterion, output);
        }
        if (!computed) {
            return false;
        }
        if (!_device->_treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=cpu_fft_%s model=%s assignment=%s grid=%d "
                    "local_grid=%d neighbors=%d\n",
                    _treePmPrecision.c_str(), _treePmModel.c_str(), _treePmAssignment.c_str(),
                    std::clamp(_treePmGridSize, 32, 128), _treePmLocalGrid ? 1 : 0,
                    std::clamp(_treePmMaxLocalNeighbors, 0, 256));
            _device->_treePmMarkerPrinted = true;
        }
        return true;
    }

    _octree.build(state);
    output.resize(state.size());
#pragma omp parallel for schedule(static) if (!_deterministicMode)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(state.size()); ++i) {
        output[static_cast<std::size_t>(i)] = _octree.computeForceOn(
            state[static_cast<std::size_t>(i)], static_cast<std::size_t>(i), forceLaw,
            _octreeOpeningCriterion);
        output[static_cast<std::size_t>(i)] =
            clampAcceleration(output[static_cast<std::size_t>(i)], _physicsMaxAcceleration);
    }
    return true;
}
