/*
 * @file engine/src/physics/TreePmCpu.hpp
 * @brief Private CPU TreePM implementation contract.
 */

#ifndef BLITZAR_ENGINE_SRC_PHYSICS_TREEPMCPU_HPP_
#define BLITZAR_ENGINE_SRC_PHYSICS_TREEPMCPU_HPP_

#include "physics/ForceLawPolicy.hpp"
#include "physics/Octree.hpp"
#include "physics/Particle.hpp"
#include "physics/Vector.hpp"

#include <complex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template <typename Scalar>
struct CpuTreePmWorkspaceT final {
    int gridSize = 0;
    bool fieldValid = false;
    Scalar originX = static_cast<Scalar>(0.0);
    Scalar originY = static_cast<Scalar>(0.0);
    Scalar originZ = static_cast<Scalar>(0.0);
    Scalar cellSize = static_cast<Scalar>(0.0);
    Scalar shortRangeScale = static_cast<Scalar>(0.0);
    bool correctionEnabled = false;
    bool treeCorrection = false;
    int maxNeighbors = 0;
    Scalar cutoffSquared = static_cast<Scalar>(0.0);
    int cellRadius = 1;
    std::vector<Scalar> density;
    std::vector<std::complex<Scalar>> spectrum;
    std::vector<std::complex<Scalar>> spectrumX;
    std::vector<std::complex<Scalar>> spectrumY;
    std::vector<std::complex<Scalar>> spectrumZ;
    std::vector<Scalar> fieldX;
    std::vector<Scalar> fieldY;
    std::vector<Scalar> fieldZ;
    std::vector<std::pair<int, int>> sortedCells;
    std::vector<int> cellStart;
    std::vector<int> cellEnd;
};

using CpuTreePmWorkspace = CpuTreePmWorkspaceT<float>;
using CpuTreePmFp64Workspace = CpuTreePmWorkspaceT<double>;

struct CpuTreePmParameters final {
    std::string model;
    bool localGrid = true;
    int gridSize = 64;
    float cutoffFactor = 1.0f;
    int maxLocalNeighbors = 64;
    int particleLimit = 0;
    std::string precision = "fp32";
    std::string assignment = "cic";
    bool periodic = false;
    bool densityContrast = false;
    float boxLength = 0.0f;
    float poissonCoefficient = 12.566370614359172f;
};

bool computeCpuTreePmForces(const std::vector<Particle>& particles,
                            const ForceLawPolicy& forceLaw,
                            const CpuTreePmParameters& parameters,
                            CpuTreePmWorkspace& workspace,
                            Octree& shortRangeTree,
                            OctreeOpeningCriterion openingCriterion,
                            std::vector<Vector3>& forces);

bool computeCpuTreePmForcesFp64(const std::vector<Particle>& particles,
                                const ForceLawPolicy& forceLaw,
                                const CpuTreePmParameters& parameters,
                                CpuTreePmFp64Workspace& workspace,
                                Octree& shortRangeTree,
                                OctreeOpeningCriterion openingCriterion,
                                std::vector<Vector3>& forces);

bool computeCpuTreePmForcesSelective(const std::vector<Particle>& particles,
                                     const std::vector<int>& activeIndices,
                                     const ForceLawPolicy& forceLaw,
                                     const CpuTreePmParameters& parameters,
                                     CpuTreePmWorkspace& workspace,
                                     Octree& shortRangeTree,
                                     OctreeOpeningCriterion openingCriterion,
                                     std::vector<Vector3>& forces);

bool computeCpuTreePmForcesSelectiveFp64(const std::vector<Particle>& particles,
                                         const std::vector<int>& activeIndices,
                                         const ForceLawPolicy& forceLaw,
                                         const CpuTreePmParameters& parameters,
                                         CpuTreePmFp64Workspace& workspace,
                                         Octree& shortRangeTree,
                                         OctreeOpeningCriterion openingCriterion,
                                         std::vector<Vector3>& forces);

bool computeCpuFp64PairwiseForces(const std::vector<Particle>& particles,
                                  const ForceLawPolicy& forceLaw,
                                  std::vector<Vector3>& forces);

#endif // BLITZAR_ENGINE_SRC_PHYSICS_TREEPMCPU_HPP_
