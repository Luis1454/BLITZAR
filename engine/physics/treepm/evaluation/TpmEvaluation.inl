/*
 * @file engine/physics/treepm/evaluation/TpmEvaluation.inl
 * @brief TreePM field sampling and local correction evaluation.
 */

template <typename Scalar>
Vector3 evaluateLocalCorrection(std::size_t index, Vector3 position,
                                const std::vector<Particle>& particles,
                                const CpuTreePmParameters& parameters,
                                const CpuTreePmWorkspaceT<Scalar>& workspace,
                                const Grid<Scalar>& grid, const ForceLawPolicy& shortRangeLaw)
{
    if (!workspace.correctionEnabled || workspace.maxNeighbors <= 0) {
        return Vector3();
    }
    const int centerX = coordinate(static_cast<Scalar>(position.x), grid.originX, grid);
    const int centerY = coordinate(static_cast<Scalar>(position.y), grid.originY, grid);
    const int centerZ = coordinate(static_cast<Scalar>(position.z), grid.originZ, grid);
    const int cellRadius = parameters.cutoffFactor > 1.0f ? 2 : 1;
    const int maxExamined = std::max(workspace.maxNeighbors * 4, workspace.maxNeighbors);
    const Scalar cutoffSquared = workspace.cutoffSquared;
    Vector3 correction;
    int accepted = 0;
    int examined = 0;
    for (int shell = 0; shell <= 2 && accepted < workspace.maxNeighbors && examined < maxExamined;
         ++shell) {
        if (shell > cellRadius) {
            break;
        }
        for (int dz = -2; dz <= 2 && accepted < workspace.maxNeighbors && examined < maxExamined;
             ++dz) {
            if (std::abs(dz) > shell) {
                continue;
            }
            const int z = centerZ + dz;
            if (z < 0 || z >= grid.size) {
                continue;
            }
            for (int dy = -2;
                 dy <= 2 && accepted < workspace.maxNeighbors && examined < maxExamined; ++dy) {
                if (std::abs(dy) > shell) {
                    continue;
                }
                const int y = centerY + dy;
                if (y < 0 || y >= grid.size) {
                    continue;
                }
                for (int dx = -2;
                     dx <= 2 && accepted < workspace.maxNeighbors && examined < maxExamined; ++dx) {
                    if (std::abs(dx) > shell ||
                        std::max({std::abs(dx), std::abs(dy), std::abs(dz)}) != shell) {
                        continue;
                    }
                    const int x = centerX + dx;
                    if (x < 0 || x >= grid.size) {
                        continue;
                    }
                    const int cell = gridIndex(x, y, z, grid.size);
                    const int begin = workspace.cellStart[static_cast<std::size_t>(cell)];
                    const int end = workspace.cellEnd[static_cast<std::size_t>(cell)];
                    if (begin < 0 || end <= begin) {
                        continue;
                    }
                    for (int cursor = begin; cursor < end && accepted < workspace.maxNeighbors &&
                                             examined < maxExamined;
                         ++cursor) {
                        const int otherIndex =
                            workspace.sortedCells[static_cast<std::size_t>(cursor)].second;
                        if (otherIndex == static_cast<int>(index)) {
                            continue;
                        }
                        ++examined;
                        const Particle& source = particles[static_cast<std::size_t>(otherIndex)];
                        const Vector3 delta = source.getPosition() - position;
                        const float distance2 =
                            delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                        if (static_cast<Scalar>(distance2) > cutoffSquared) {
                            continue;
                        }
                        correction += sourceAcceleration<Scalar>(position, source, shortRangeLaw);
                        ++accepted;
                    }
                }
            }
        }
    }
    return correction;
}

template <typename Scalar>
void evaluateTreePmForces(
    const std::vector<Particle>& particles, const CpuTreePmParameters& parameters,
    const CpuTreePmWorkspaceT<Scalar>& workspace, const Grid<Scalar>& grid, Octree& shortRangeTree,
    OctreeOpeningCriterion openingCriterion, const ForceLawPolicy& shortRangeLaw,
    std::vector<Vector3>& forces,
    const std::optional<std::reference_wrapper<const std::vector<int>>>& activeIndices)
{
    const std::size_t count =
        activeIndices.has_value() ? activeIndices->get().size() : particles.size();
#pragma omp parallel for schedule(static)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(count); ++i) {
        const std::size_t index =
            !activeIndices.has_value()
                ? static_cast<std::size_t>(i)
                : static_cast<std::size_t>(activeIndices->get()[static_cast<std::size_t>(i)]);
        const Vector3 position = particles[index].getPosition();
        forces[index] = Vector3(
            static_cast<float>(sample(workspace.fieldX, grid, position, parameters.assignment)),
            static_cast<float>(sample(workspace.fieldY, grid, position, parameters.assignment)),
            static_cast<float>(sample(workspace.fieldZ, grid, position, parameters.assignment)));
        if (workspace.treeCorrection) {
            forces[index] += shortRangeTree.computeForceOn(
                particles[index], index, shortRangeLaw, openingCriterion,
                static_cast<float>(workspace.cutoffSquared));
            continue;
        }
        forces[index] += evaluateLocalCorrection(index, position, particles, parameters, workspace,
                                                 grid, shortRangeLaw);
    }
}
