#include "solvers/fmm/kifmm/KifmmSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace blitzar_kifmm {

namespace {

[[nodiscard]] blitzar_core::Vector3 ScalePoint(
    blitzar_core::Vector3 point, blitzar_core::Scalar half_extent) noexcept
{
    return {point.x * half_extent, point.y * half_extent, point.z * half_extent};
}

[[nodiscard]] blitzar_core::Vector3 Difference(
    blitzar_core::Vector3 left, blitzar_core::Vector3 right) noexcept
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

[[nodiscard]] blitzar_core::Scalar SquaredLength(blitzar_core::Vector3 value) noexcept
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

[[nodiscard]] bool InvertMatrix(
    std::span<blitzar_core::Scalar> matrix, std::span<blitzar_core::Scalar> inverse) noexcept
{
    constexpr std::size_t node_count = KifmmWorkspace::SurfaceNodeCount;
    constexpr std::size_t matrix_size = node_count * node_count;

    if (matrix.size() != matrix_size || inverse.size() != matrix_size) {
        return false;
    }

    blitzar_core::Scalar scale = 0.0;

    for (const blitzar_core::Scalar value : matrix) {
        if (!std::isfinite(value)) {
            return false;
        }

        scale = std::max(scale, std::abs(value));
    }

    const blitzar_core::Scalar tolerance =
        std::numeric_limits<blitzar_core::Scalar>::epsilon() * 4096.0 * std::max(1.0, scale);

    std::array<std::size_t, node_count> swaps{};

    for (std::size_t pivot = 0; pivot < node_count; ++pivot) {
        std::size_t pivot_row = pivot;
        blitzar_core::Scalar pivot_size = std::abs(matrix[pivot * node_count + pivot]);

        for (std::size_t row = pivot + 1; row < node_count; ++row) {
            const blitzar_core::Scalar candidate = std::abs(matrix[row * node_count + pivot]);

            if (candidate > pivot_size) {
                pivot_row = row;
                pivot_size = candidate;
            }
        }

        if (!std::isfinite(pivot_size) || pivot_size <= tolerance) {
            return false;
        }

        swaps[pivot] = pivot_row;

        if (pivot_row != pivot) {
            for (std::size_t column = 0; column < node_count; ++column) {
                std::swap(
                    matrix[pivot * node_count + column], matrix[pivot_row * node_count + column]);
            }
        }

        const blitzar_core::Scalar diagonal = matrix[pivot * node_count + pivot];

        for (std::size_t row = pivot + 1; row < node_count; ++row) {
            const blitzar_core::Scalar factor = matrix[row * node_count + pivot] / diagonal;

            matrix[row * node_count + pivot] = factor;

            for (std::size_t column = pivot + 1; column < node_count; ++column) {
                matrix[row * node_count + column] -= factor * matrix[pivot * node_count + column];
            }
        }
    }

    for (std::size_t column = 0; column < node_count; ++column) {
        std::array<blitzar_core::Scalar, node_count> right_hand_side{};

        right_hand_side[column] = 1.0;

        for (std::size_t pivot = 0; pivot < node_count; ++pivot) {
            if (swaps[pivot] != pivot) {
                std::swap(right_hand_side[pivot], right_hand_side[swaps[pivot]]);
            }
        }

        for (std::size_t row = 0; row < node_count; ++row) {
            for (std::size_t lower = 0; lower < row; ++lower) {
                right_hand_side[row] -= matrix[row * node_count + lower] * right_hand_side[lower];
            }
        }

        for (std::size_t row = node_count; row-- > 0;) {
            for (std::size_t upper = row + 1; upper < node_count; ++upper) {
                right_hand_side[row] -= matrix[row * node_count + upper] * right_hand_side[upper];
            }

            const blitzar_core::Scalar diagonal = matrix[row * node_count + row];

            if (!std::isfinite(diagonal) || std::abs(diagonal) <= tolerance) {
                return false;
            }

            right_hand_side[row] /= diagonal;
        }

        for (std::size_t row = 0; row < node_count; ++row) {
            if (!std::isfinite(right_hand_side[row])) {
                return false;
            }

            inverse[row * node_count + column] = right_hand_side[row];
        }
    }

    return true;
}

} // namespace

blitzar_status KifmmSolver::BuildOperators(const blitzar_trees::OctreeView& tree) noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = tree.Cells();

    if (cells.empty() || cells.size() > workspace_.CellCapacity()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::size_t deepest = 0;

    for (const blitzar_trees::Octree::Cell& cell : cells) {
        if (!std::isfinite(cell.half_extent) || cell.half_extent <= 0.0 ||
            cell.depth > workspace_.MaxDepth()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        deepest = std::max(deepest, cell.depth);
    }

    blitzar_core::Scalar half_extent = cells.front().half_extent;
    const blitzar_core::Scalar softening = parameters_.EffectiveSoftening();

    for (std::size_t depth = 0; depth <= deepest; ++depth) {
        if (!std::isfinite(half_extent) || half_extent <= 0.0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_status status = BuildOperator(depth, half_extent, softening);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (depth != deepest) {
            half_extent *= 0.5;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::BuildOperator(
    std::size_t depth, blitzar_core::Scalar half_extent, blitzar_core::Scalar softening) noexcept
{
    constexpr std::size_t node_count = KifmmWorkspace::SurfaceNodeCount;
    constexpr std::size_t matrix_size = node_count * node_count;
    const std::span<const blitzar_core::Vector3> equivalent_nodes = workspace_.EquivalentNodes();
    const std::span<const blitzar_core::Vector3> check_nodes = workspace_.CheckNodes();
    const std::span<blitzar_core::Scalar> matrix = workspace_.Operators(depth);
    std::array<blitzar_core::Scalar, matrix_size> inverse{};

    if (equivalent_nodes.size() != node_count || check_nodes.size() != node_count ||
        matrix.size() != matrix_size || !std::isfinite(softening) || softening < 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_core::Scalar softening_squared = softening * softening;

    if (!std::isfinite(softening_squared)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t row = 0; row < node_count; ++row) {
        const blitzar_core::Vector3 check = ScalePoint(check_nodes[row], half_extent);

        for (std::size_t column = 0; column < node_count; ++column) {
            const blitzar_core::Vector3 equivalent =
                ScalePoint(equivalent_nodes[column], half_extent);

            const blitzar_core::Scalar squared_distance =
                SquaredLength(Difference(equivalent, check));

            const blitzar_core::Scalar softened_distance = squared_distance + softening_squared;

            if (!std::isfinite(softened_distance) || softened_distance <= 0.0) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            matrix[row * node_count + column] = 1.0 / std::sqrt(softened_distance);
        }
    }

    if (!InvertMatrix(matrix, inverse)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::copy(inverse.begin(), inverse.end(), matrix.begin());

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::BuildCellWeights(
    const TreeComputeRequest& request, std::size_t cell_index) noexcept
{
    constexpr std::size_t node_count = KifmmWorkspace::SurfaceNodeCount;
    const std::span<const blitzar_trees::Octree::Cell> cells = request.tree.Cells();
    const std::span<blitzar_core::Scalar> weights = workspace_.CellWeights(cell_index);

    if (cell_index >= cells.size() || weights.size() != node_count) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const blitzar_trees::Octree::Cell& cell = cells[cell_index];
    const std::size_t source_count = request.sources.SourceCount();

    if (cell.begin > source_count || cell.count > source_count - cell.begin) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const std::span<const blitzar_core::Vector3> check_nodes = workspace_.CheckNodes();
    const std::span<blitzar_core::Scalar> operator_matrix = workspace_.Operators(cell.depth);
    std::array<blitzar_core::Scalar, node_count> samples{};
    const blitzar_core::Scalar softening = parameters_.EffectiveSoftening();
    const blitzar_core::Scalar softening_squared = softening * softening;

    std::fill(weights.begin(), weights.end(), 0.0);

    if (check_nodes.size() != node_count || operator_matrix.size() != node_count * node_count ||
        !std::isfinite(softening_squared)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    for (std::size_t check = 0; check < node_count; ++check) {
        const blitzar_core::Vector3 check_position = {
            cell.center.x + cell.half_extent * check_nodes[check].x,
            cell.center.y + cell.half_extent * check_nodes[check].y,
            cell.center.z + cell.half_extent * check_nodes[check].z};

        for (std::size_t offset = 0; offset < cell.count; ++offset) {
            if (cell.begin > std::numeric_limits<std::size_t>::max() - offset) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            std::size_t source = 0;

            if (!request.tree.ParticleIndex(cell.begin + offset, source) ||
                source >= source_count) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            const blitzar_core::Scalar mass = request.sources.mass[source];

            if (mass == 0.0) {
                continue;
            }

            const blitzar_core::Vector3 displacement{request.sources.x[source] - check_position.x,
                request.sources.y[source] - check_position.y,
                request.sources.z[source] - check_position.z};

            const blitzar_core::Scalar softened_distance =
                SquaredLength(displacement) + softening_squared;

            if (!std::isfinite(softened_distance) || softened_distance <= 0.0) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            samples[check] += mass / std::sqrt(softened_distance);

            if (!std::isfinite(samples[check])) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }
    }

    for (std::size_t node = 0; node < node_count; ++node) {
        for (std::size_t check = 0; check < node_count; ++check) {
            weights[node] += operator_matrix[node * node_count + check] * samples[check];
        }

        if (!std::isfinite(weights[node])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status KifmmSolver::BuildEquivalentWeights(const TreeComputeRequest& request) noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = request.tree.Cells();

    if (cells.empty() || cells.size() > workspace_.CellCapacity()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t cell = 0; cell < cells.size(); ++cell) {
        const blitzar_status status = BuildCellWeights(request, cell);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_kifmm
