/*
 * @file engine/physics/treepm/preparation/TpmPreparation.inl
 * @brief TreePM field and short-range index preparation.
 */

template <typename Scalar>
bool rebuildTreePmField(const std::vector<Particle>& particles, const ForceLawPolicy& forceLaw,
                        const CpuTreePmParameters& parameters,
                        CpuTreePmWorkspaceT<Scalar>& workspace, Grid<Scalar>& grid,
                        bool correctionEnabled, bool treeCorrection, int maxNeighbors)
{
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
    const Scalar extent =
        std::max({maxX - minX, maxY - minY, maxZ - minZ, static_cast<Scalar>(forceLaw.softening)});
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
    for (const Particle& particle : particles) {
        depositParticle(particle, grid, workspace.density, parameters.assignment);
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
    return true;
}

template <typename Scalar>
bool restoreTreePmGrid(const CpuTreePmWorkspaceT<Scalar>& workspace, Grid<Scalar>& grid,
                       bool periodic)
{
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
    grid.periodic = periodic;
    return true;
}

template <typename Scalar>
void prepareTreePmCorrection(const std::vector<Particle>& particles,
                             const CpuTreePmParameters& parameters,
                             CpuTreePmWorkspaceT<Scalar>& workspace, const Grid<Scalar>& grid,
                             bool rebuildField, bool correctionEnabled, bool treeCorrection,
                             int maxNeighbors, Octree& shortRangeTree)
{
    if (!rebuildField) {
        return;
    }
    workspace.sortedCells.clear();
    workspace.cellStart.clear();
    workspace.cellEnd.clear();
    if (correctionEnabled && !treeCorrection && maxNeighbors > 0) {
        workspace.sortedCells.reserve(particles.size());
        for (int i = 0; i < static_cast<int>(particles.size()); ++i) {
            const Vector3 position = particles[static_cast<std::size_t>(i)].getPosition();
            workspace.sortedCells.emplace_back(cellHash<Scalar>(position, grid), i);
        }
        std::sort(workspace.sortedCells.begin(), workspace.sortedCells.end());
        const std::size_t cellCount = static_cast<std::size_t>(grid.size) * grid.size * grid.size;
        workspace.cellStart.assign(cellCount, -1);
        workspace.cellEnd.assign(cellCount, -1);
        for (int i = 0; i < static_cast<int>(workspace.sortedCells.size()); ++i) {
            const int cell = workspace.sortedCells[static_cast<std::size_t>(i)].first;
            if (i == 0 || workspace.sortedCells[static_cast<std::size_t>(i - 1)].first != cell) {
                workspace.cellStart[static_cast<std::size_t>(cell)] = i;
            }
            if (i + 1 == static_cast<int>(workspace.sortedCells.size()) ||
                workspace.sortedCells[static_cast<std::size_t>(i + 1)].first != cell) {
                workspace.cellEnd[static_cast<std::size_t>(cell)] = i + 1;
            }
        }
    }
    if (treeCorrection) {
        shortRangeTree.build(particles);
    }
}
