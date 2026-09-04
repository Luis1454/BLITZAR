#include "solvers/fmm/kifmm/KifmmSolver.hpp"

#include <cmath>
#include <limits>

namespace blitzar_kifmm {

namespace {

[[nodiscard]] blitzar_core::Vector3 Position(
    const blitzar_core::ParticleStateView& particles, std::size_t index) noexcept
{
    return {particles.x[index], particles.y[index], particles.z[index]};
}

[[nodiscard]] bool Contains(
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

[[nodiscard]] blitzar_core::Scalar SquaredLength(blitzar_core::Vector3 value) noexcept
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

[[nodiscard]] blitzar_core::Vector3 Difference(
    blitzar_core::Vector3 left, blitzar_core::Vector3 right) noexcept
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

} // namespace

blitzar_status KifmmSolver::BuildInteractions(
    const TreeComputeRequest& request, std::size_t target, InteractionList& list) noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = request.tree.Cells();

    if (cells.empty() || target >= request.targets.count || list.stack.empty() ||
        list.near_cells.size() < cells.size() || list.far_cells.size() < cells.size()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    list.near_count = 0;
    list.far_count = 0;

    std::size_t stack_size = 1;

    list.stack[0] = 0;

    const blitzar_core::Vector3 target_position = Position(request.targets, target);

    while (stack_size > 0) {
        const std::size_t cell_index = list.stack[--stack_size];

        if (cell_index >= cells.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_trees::Octree::Cell& cell = cells[cell_index];

        if (cell.IsLeaf()) {
            if (list.near_count == list.near_cells.size()) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            list.near_cells[list.near_count++] = cell_index;

            continue;
        }

        const blitzar_core::Vector3 displacement = Difference(cell.center_of_mass, target_position);
        const blitzar_core::Scalar squared_distance = SquaredLength(displacement);
        const blitzar_core::Scalar distance = std::sqrt(squared_distance);

        if (!Contains(cell, target_position) && distance > 0.0 &&
            2.0 * cell.half_extent / distance < settings_.opening_angle) {
            if (list.far_count == list.far_cells.size()) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            list.far_cells[list.far_count++] = cell_index;

            continue;
        }

        for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
            if (*child == blitzar_trees::Octree::Cell::InvalidIndex) {
                continue;
            }
            if (*child >= cells.size() || stack_size == list.stack.size()) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            list.stack[stack_size++] = *child;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::AccumulateNearCell(const TreeComputeRequest& request,
    std::size_t target, std::size_t cell_index, blitzar_core::Vector3& acceleration) const noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = request.tree.Cells();

    if (cell_index >= cells.size()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const blitzar_trees::Octree::Cell& cell = cells[cell_index];
    const std::size_t source_count = request.sources.SourceCount();
    const blitzar_core::Vector3 target_position = Position(request.targets, target);

    if (cell.begin > source_count || cell.count > source_count - cell.begin) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        if (cell.begin > std::numeric_limits<std::size_t>::max() - offset) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        std::size_t source = 0;

        if (!request.tree.ParticleIndex(cell.begin + offset, source) || source >= source_count) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if ((request.skip_self && source == target) || request.sources.mass[source] == 0.0) {
            continue;
        }

        const blitzar_core::Vector3 displacement =
            Difference(Position(request.sources, source), target_position);

        const blitzar_core::Scalar squared_distance = SquaredLength(displacement);
        const blitzar_physics::PairStatus pair_status =
            gravity_.ValidatePair(request.sources.mass[source], squared_distance);

        const blitzar_status status = PairStatusToStatus(pair_status);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        const blitzar_core::Scalar factor =
            gravity_.PairFactor(request.sources.mass[source], squared_distance);

        if (!std::isfinite(factor)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        acceleration.x += factor * displacement.x;
        acceleration.y += factor * displacement.y;
        acceleration.z += factor * displacement.z;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::AccumulateFarCell(const TreeComputeRequest& request,
    const blitzar_core::Vector3& target_position, std::size_t cell_index,
    blitzar_core::Vector3& acceleration) const noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = request.tree.Cells();
    const std::span<const blitzar_core::Vector3> equivalent_nodes = workspace_.EquivalentNodes();
    const std::span<const blitzar_core::Scalar> weights = workspace_.CellWeights(cell_index);

    if (cell_index >= cells.size() || equivalent_nodes.size() != KifmmWorkspace::SurfaceNodeCount ||
        weights.size() != KifmmWorkspace::SurfaceNodeCount) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const blitzar_trees::Octree::Cell& cell = cells[cell_index];
    const blitzar_core::Scalar softening = parameters_.EffectiveSoftening();
    const blitzar_core::Scalar softening_squared = softening * softening;
    const blitzar_core::Scalar constant = parameters_.EffectiveConstant();

    if (!std::isfinite(softening_squared) || !std::isfinite(constant) || constant <= 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t node = 0; node < equivalent_nodes.size(); ++node) {
        const blitzar_core::Scalar weight = weights[node];

        if (!std::isfinite(weight) || weight == 0.0) {
            continue;
        }

        const blitzar_core::Vector3 point{
            cell.center.x + cell.half_extent * equivalent_nodes[node].x,
            cell.center.y + cell.half_extent * equivalent_nodes[node].y,
            cell.center.z + cell.half_extent * equivalent_nodes[node].z};

        const blitzar_core::Vector3 displacement = Difference(point, target_position);
        const blitzar_core::Scalar softened_distance =
            SquaredLength(displacement) + softening_squared;

        if (!std::isfinite(softened_distance) || softened_distance <= 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_core::Scalar factor =
            constant * weight / (softened_distance * std::sqrt(softened_distance));

        if (!std::isfinite(factor)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        acceleration.x += factor * displacement.x;
        acceleration.y += factor * displacement.y;
        acceleration.z += factor * displacement.z;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::ComputeTarget(
    const TreeComputeRequest& request, std::size_t target) noexcept
{
    InteractionList list{
        workspace_.TraversalStack(), workspace_.NearInteractions(), workspace_.FarInteractions()};

    const blitzar_status interaction_status = BuildInteractions(request, target, list);

    if (interaction_status != BLITZAR_STATUS_OK) {
        return interaction_status;
    }

    const blitzar_core::Vector3 target_position = Position(request.targets, target);
    blitzar_core::Vector3 acceleration{};

    for (std::size_t index = 0; index < list.near_count; ++index) {
        const blitzar_status status =
            AccumulateNearCell(request, target, list.near_cells[index], acceleration);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    for (std::size_t index = 0; index < list.far_count; ++index) {
        const blitzar_status status =
            AccumulateFarCell(request, target_position, list.far_cells[index], acceleration);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    if (!std::isfinite(acceleration.x) || !std::isfinite(acceleration.y) ||
        !std::isfinite(acceleration.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (request.accumulate) {
        workspace_.Staging()[target].x += acceleration.x;
        workspace_.Staging()[target].y += acceleration.y;
        workspace_.Staging()[target].z += acceleration.z;
    }
    else {
        workspace_.Staging()[target] = acceleration;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::ComputeTargets(const TreeComputeRequest& request) noexcept
{
    for (std::size_t target = 0; target < request.targets.count; ++target) {
        const blitzar_status status = ComputeTarget(request, target);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::ComputeTree(const TreeComputeRequest& request) noexcept
{
    if (!ValidateTreeRequest(request)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (request.accumulate) {
        for (std::size_t target = 0; target < request.targets.count; ++target) {
            const blitzar_core::Vector3 force{
                request.forces.x[target], request.forces.y[target], request.forces.z[target]};

            if (!std::isfinite(force.x) || !std::isfinite(force.y) || !std::isfinite(force.z)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            workspace_.Staging()[target] = force;
        }
    }

    if (request.sources.SourceCount() == 0) {
        if (!request.accumulate) {
            for (std::size_t target = 0; target < request.targets.count; ++target) {
                workspace_.Staging()[target] = {};
            }
        }

        return BLITZAR_STATUS_OK;
    }

    if (settings_.opening_angle != 0.0) {
        const blitzar_status operator_status = BuildOperators(request.tree);

        if (operator_status != BLITZAR_STATUS_OK) {
            return operator_status;
        }

        const blitzar_status weight_status = BuildEquivalentWeights(request);

        if (weight_status != BLITZAR_STATUS_OK) {
            return weight_status;
        }
    }

    return ComputeTargets(request);
}

blitzar_status KifmmSolver::CommitStagedForces(blitzar_core::ForceView forces) noexcept
{
    const std::span<blitzar_core::Vector3> staging = workspace_.Staging();

    if (!blitzar_core::IsValid(forces) || forces.count > staging.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < forces.count; ++index) {
        forces.x[index] = staging[index].x;
        forces.y[index] = staging[index].y;
        forces.z[index] = staging[index].z;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_kifmm
