/*
 * @file engine/physics/treepm/force/TpmForce.inl
 * @brief Private TreePM force orchestration.
 */

template <typename Scalar>
bool computeCpuTreePmForcesTyped(
    const std::vector<Particle>& particles, const ForceLawPolicy& forceLaw,
    const CpuTreePmParameters& parameters, CpuTreePmWorkspaceT<Scalar>& workspace,
    Octree& shortRangeTree, OctreeOpeningCriterion openingCriterion, std::vector<Vector3>& forces,
    const std::optional<std::reference_wrapper<const std::vector<int>>>& activeIndices)
{
    if (particles.empty()) {
        return false;
    }

    const bool correctionEnabled = !parameters.periodic && parameters.model != "pm_only" &&
                                   (parameters.localGrid || parameters.model == "tree" ||
                                    parameters.model == "hybrid" || parameters.model == "auto");
    const bool treeCorrection =
        !parameters.periodic && (parameters.model == "tree" || parameters.model == "hybrid");
    const int maxNeighbors = std::clamp(parameters.maxLocalNeighbors, 0, 256);
    const bool rebuildField = !activeIndices.has_value() || !workspace.fieldValid;
    Grid<Scalar> grid;
    const bool fieldReady =
        rebuildField ? rebuildTreePmField(particles, forceLaw, parameters, workspace, grid,
                                          correctionEnabled, treeCorrection, maxNeighbors)
                     : restoreTreePmGrid(workspace, grid, parameters.periodic);
    if (!fieldReady) {
        return false;
    }

    prepareTreePmCorrection(particles, parameters, workspace, grid, rebuildField, correctionEnabled,
                            treeCorrection, maxNeighbors, shortRangeTree);
    ForceLawPolicy shortRangeLaw = forceLaw;
    shortRangeLaw.treePmShortRangeScale = static_cast<float>(workspace.shortRangeScale);
    if (forces.size() != particles.size()) {
        forces.assign(particles.size(), Vector3());
    }
    evaluateTreePmForces(particles, parameters, workspace, grid, shortRangeTree, openingCriterion,
                         shortRangeLaw, forces, activeIndices);
    return true;
}
