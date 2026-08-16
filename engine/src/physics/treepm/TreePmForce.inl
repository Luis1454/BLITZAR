/*
 * @file engine/src/physics/treepm/TreePmForce.inl
 * @brief Private TreePM short-range force orchestration.
 *
 * This fragment is included by TreePmCpu.cpp inside the implementation
 * namespace so the scalar precision variants share one implementation.
 */

template <typename Scalar>
Vector3 sourceAcceleration(Vector3 self, const Particle& source, const ForceLawPolicy& policy)
{
    const Vector3 delta = source.getPosition() - self;
    const Scalar distance2 = static_cast<Scalar>(delta.x) * delta.x +
                             static_cast<Scalar>(delta.y) * delta.y +
                             static_cast<Scalar>(delta.z) * delta.z +
                             static_cast<Scalar>(policy.softening) * policy.softening;
    if (distance2 <= static_cast<Scalar>(policy.minDistance2)) {
        return Vector3();
    }
    const Scalar inverseDistance = static_cast<Scalar>(1.0) / std::sqrt(distance2);
    Scalar shortRangeWeight = static_cast<Scalar>(1.0);
    if (policy.treePmShortRangeScale > 0.0f) {
        const Scalar distance = static_cast<Scalar>(1.0) / inverseDistance;
        const Scalar splitScale = static_cast<Scalar>(policy.treePmShortRangeScale);
        const Scalar argument = static_cast<Scalar>(0.5) * distance / splitScale;
        shortRangeWeight =
            std::erfc(argument) +
            distance / (splitScale * std::sqrt(static_cast<Scalar>(3.14159265358979323846))) *
                std::exp(-argument * argument);
    }
    const Scalar scale = static_cast<Scalar>(source.getMass()) * inverseDistance * inverseDistance *
                         inverseDistance * shortRangeWeight;
    return Vector3(static_cast<float>(static_cast<Scalar>(delta.x) * scale),
                   static_cast<float>(static_cast<Scalar>(delta.y) * scale),
                   static_cast<float>(static_cast<Scalar>(delta.z) * scale));
}

template <typename Scalar>
bool computeCpuTreePmForcesTyped(const std::vector<Particle>& particles,
                                 const ForceLawPolicy& forceLaw,
                                 const CpuTreePmParameters& parameters,
                                 CpuTreePmWorkspaceT<Scalar>& workspace, Octree& shortRangeTree,
                                 OctreeOpeningCriterion openingCriterion,
                                 std::vector<Vector3>& forces,
                                 const std::optional<std::reference_wrapper<const std::vector<int>>>&
                                     activeIndices)
{
    if (particles.empty()) {
        return false;
    }
    Grid<Scalar> grid;
    const bool correctionEnabled = !parameters.periodic && parameters.model != "pm_only" &&
                                   (parameters.localGrid || parameters.model == "tree" ||
                                    parameters.model == "hybrid" || parameters.model == "auto");
    const bool treeCorrection =
        !parameters.periodic && (parameters.model == "tree" || parameters.model == "hybrid");
    const int maxNeighbors = std::clamp(parameters.maxLocalNeighbors, 0, 256);
    const bool rebuildField = !activeIndices.has_value() || !workspace.fieldValid;
    if (rebuildField) {
        const int requestedGridSize = std::clamp(parameters.gridSize, 32, 128);
        Scalar minX = static_cast<Scalar>(particles.front().getPosition().x);
        Scalar minY = static_cast<Scalar>(particles.front().getPosition().y);
        Scalar minZ = static_cast<Scalar>(particles.front().getPosition().z);
        Scalar maxX = minX;
        Scalar maxY = minY;
        Scalar maxZ = minZ;
        for (const Particle& particle : particles) {
            const Vector3 position = particle.getPosition();
            minX = std::min(minX, static_cast<Scalar>(position.x));
            minY = std::min(minY, static_cast<Scalar>(position.y));
            minZ = std::min(minZ, static_cast<Scalar>(position.z));
            maxX = std::max(maxX, static_cast<Scalar>(position.x));
            maxY = std::max(maxY, static_cast<Scalar>(position.y));
            maxZ = std::max(maxZ, static_cast<Scalar>(position.z));
        }
        const Scalar extent = std::max(
            {maxX - minX, maxY - minY, maxZ - minZ, static_cast<Scalar>(forceLaw.softening)});
        grid.periodic = parameters.periodic;
        grid.size = parameters.periodic ? requestedGridSize : requestedGridSize * 2;
        const Scalar periodicLength =
            std::max(static_cast<Scalar>(parameters.boxLength), static_cast<Scalar>(1.0e-6));
        grid.cellSize = parameters.periodic
                            ? periodicLength / static_cast<Scalar>(grid.size)
                            : std::max(static_cast<Scalar>(0.25),
                                       extent / static_cast<Scalar>(requestedGridSize - 2));
        grid.inverseCellSize = static_cast<Scalar>(1.0) / grid.cellSize;
        const Scalar halfExtent =
            static_cast<Scalar>(0.5) * static_cast<Scalar>(grid.size) * grid.cellSize;
        grid.originX = parameters.periodic ? static_cast<Scalar>(0.0)
                                           : static_cast<Scalar>(0.5) * (minX + maxX) - halfExtent;
        grid.originY = parameters.periodic ? static_cast<Scalar>(0.0)
                                           : static_cast<Scalar>(0.5) * (minY + maxY) - halfExtent;
        grid.originZ = parameters.periodic ? static_cast<Scalar>(0.0)
                                           : static_cast<Scalar>(0.5) * (minZ + maxZ) - halfExtent;

        const std::size_t cellCount = static_cast<std::size_t>(grid.size) * grid.size * grid.size;
        workspace.gridSize = grid.size;
        workspace.originX = grid.originX;
        workspace.originY = grid.originY;
        workspace.originZ = grid.originZ;
        workspace.cellSize = grid.cellSize;
        const Scalar cutoff = std::clamp(static_cast<Scalar>(parameters.cutoffFactor),
                                         static_cast<Scalar>(1.0), static_cast<Scalar>(2.0)) *
                              grid.cellSize;
        workspace.shortRangeScale =
            parameters.periodic ? static_cast<Scalar>(0.0) : cutoff / static_cast<Scalar>(4.5);
        workspace.correctionEnabled = correctionEnabled;
        workspace.treeCorrection = treeCorrection;
        workspace.maxNeighbors = maxNeighbors;
        workspace.cutoffSquared = cutoff * cutoff;
        workspace.cellRadius = parameters.cutoffFactor > 1.0f ? 2 : 1;
        workspace.density.assign(cellCount, static_cast<Scalar>(0.0));
        // TreePM represents the full mass distribution; sampling sources would
        // solve a different gravitational system.
        const std::size_t limit = particles.size();
        for (std::size_t i = 0; i < limit; ++i) {
            depositParticle(particles[i], grid, workspace.density, parameters.assignment);
        }
        if (parameters.densityContrast) {
            const Scalar boxVolume = periodicLength * periodicLength * periodicLength;
            Scalar totalMass = static_cast<Scalar>(0.0);
            for (const Particle& particle : particles) {
                totalMass += static_cast<Scalar>(particle.getMass());
            }
            const Scalar meanDensity = totalMass / boxVolume;
            if (meanDensity <= static_cast<Scalar>(0.0)) {
                return false;
            }
            for (Scalar& density : workspace.density) {
                density = density / meanDensity - static_cast<Scalar>(1.0);
            }
        }
        buildFftFields<Scalar>(grid, workspace.shortRangeScale,
                               static_cast<Scalar>(parameters.poissonCoefficient),
                               parameters.assignment, workspace);
        workspace.fieldValid = true;
    }
    else {
        if (workspace.gridSize <= 0 || workspace.fieldX.empty() || workspace.fieldY.empty() ||
            workspace.fieldZ.empty()) {
            return false;
        }
        grid.size = workspace.gridSize;
        grid.originX = workspace.originX;
        grid.originY = workspace.originY;
        grid.originZ = workspace.originZ;
        grid.cellSize = workspace.cellSize;
        grid.inverseCellSize = static_cast<Scalar>(1.0) / grid.cellSize;
        grid.periodic = parameters.periodic;
    }

    const std::size_t cellCount = static_cast<std::size_t>(grid.size) * grid.size * grid.size;
    const Scalar cutoffSquared = workspace.cutoffSquared;
    ForceLawPolicy shortRangeLaw = forceLaw;
    shortRangeLaw.treePmShortRangeScale = static_cast<float>(workspace.shortRangeScale);
    if (forces.size() != particles.size()) {
        forces.assign(particles.size(), Vector3());
    }
    std::vector<std::pair<int, int>>& sortedCells = workspace.sortedCells;
    std::vector<int>& cellStart = workspace.cellStart;
    std::vector<int>& cellEnd = workspace.cellEnd;
    if (rebuildField) {
        sortedCells.clear();
        cellStart.clear();
        cellEnd.clear();
    }
    if (rebuildField && correctionEnabled && !treeCorrection && maxNeighbors > 0) {
        sortedCells.reserve(particles.size());
        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            const Vector3 position = particles[static_cast<std::size_t>(i)].getPosition();
            sortedCells.emplace_back(cellHash<Scalar>(position, grid), i);
        }
        std::sort(sortedCells.begin(), sortedCells.end());
        cellStart.assign(cellCount, -1);
        cellEnd.assign(cellCount, -1);
        for (int i = 0; i < static_cast<int>(sortedCells.size()); ++i) {
            const int cell = sortedCells[static_cast<std::size_t>(i)].first;
            if (i == 0 || sortedCells[static_cast<std::size_t>(i - 1)].first != cell) {
                cellStart[static_cast<std::size_t>(cell)] = i;
            }
            if (i + 1 == static_cast<int>(sortedCells.size()) ||
                sortedCells[static_cast<std::size_t>(i + 1)].first != cell) {
                cellEnd[static_cast<std::size_t>(cell)] = i + 1;
            }
        }
    }
    if (rebuildField && treeCorrection) {
        shortRangeTree.build(particles);
    }

#pragma omp parallel for schedule(static)
    for (std::ptrdiff_t i = 0;
         i < static_cast<std::ptrdiff_t>(activeIndices.has_value()
                                             ? activeIndices->get().size()
                                             : particles.size());
         ++i) {
        const std::size_t index =
            !activeIndices.has_value()
                ? static_cast<std::size_t>(i)
                : static_cast<std::size_t>(
                      activeIndices->get()[static_cast<std::size_t>(i)]);
        const Vector3 position = particles[index].getPosition();
        forces[index] = Vector3(
            static_cast<float>(sample(workspace.fieldX, grid, position, parameters.assignment)),
            static_cast<float>(sample(workspace.fieldY, grid, position, parameters.assignment)),
            static_cast<float>(sample(workspace.fieldZ, grid, position, parameters.assignment)));
        if (treeCorrection) {
            forces[index] +=
                shortRangeTree.computeForceOn(particles[index], index, shortRangeLaw,
                                              openingCriterion, static_cast<float>(cutoffSquared));
            continue;
        }
        if (!correctionEnabled || maxNeighbors <= 0) {
            continue;
        }
        const int centerX = coordinate(static_cast<Scalar>(position.x), grid.originX, grid);
        const int centerY = coordinate(static_cast<Scalar>(position.y), grid.originY, grid);
        const int centerZ = coordinate(static_cast<Scalar>(position.z), grid.originZ, grid);
        const int cellRadius = parameters.cutoffFactor > 1.0f ? 2 : 1;
        const int maxExamined = std::max(maxNeighbors * 4, maxNeighbors);
        int accepted = 0;
        int examined = 0;
        for (int shell = 0; shell <= 2 && accepted < maxNeighbors && examined < maxExamined;
             ++shell) {
            if (shell > cellRadius) {
                break;
            }
            for (int dz = -2; dz <= 2 && accepted < maxNeighbors && examined < maxExamined; ++dz) {
                if (std::abs(dz) > shell) {
                    continue;
                }
                const int z = centerZ + dz;
                if (z < 0 || z >= grid.size) {
                    continue;
                }
                for (int dy = -2; dy <= 2 && accepted < maxNeighbors && examined < maxExamined;
                     ++dy) {
                    if (std::abs(dy) > shell) {
                        continue;
                    }
                    const int y = centerY + dy;
                    if (y < 0 || y >= grid.size) {
                        continue;
                    }
                    for (int dx = -2; dx <= 2 && accepted < maxNeighbors && examined < maxExamined;
                         ++dx) {
                        if (std::abs(dx) > shell ||
                            std::max({std::abs(dx), std::abs(dy), std::abs(dz)}) != shell) {
                            continue;
                        }
                        const int x = centerX + dx;
                        if (x < 0 || x >= grid.size) {
                            continue;
                        }
                        const int cell = gridIndex(x, y, z, grid.size);
                        const int begin = cellStart[static_cast<std::size_t>(cell)];
                        const int end = cellEnd[static_cast<std::size_t>(cell)];
                        if (begin < 0 || end <= begin) {
                            continue;
                        }
                        for (int cursor = begin;
                             cursor < end && accepted < maxNeighbors && examined < maxExamined;
                             ++cursor) {
                            const int otherIndex =
                                sortedCells[static_cast<std::size_t>(cursor)].second;
                            if (otherIndex == static_cast<int>(index)) {
                                continue;
                            }
                            ++examined;
                            const Particle& source =
                                particles[static_cast<std::size_t>(otherIndex)];
                            const Vector3 delta = source.getPosition() - position;
                            const float distance2 =
                                delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                            if (static_cast<Scalar>(distance2) > cutoffSquared) {
                                continue;
                            }
                            forces[index] +=
                                sourceAcceleration<Scalar>(position, source, shortRangeLaw);
                            ++accepted;
                        }
                    }
                }
            }
        }
    }
    return true;
}
