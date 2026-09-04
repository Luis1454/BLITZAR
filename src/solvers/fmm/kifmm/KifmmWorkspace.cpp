#include "solvers/fmm/kifmm/KifmmWorkspace.hpp"

#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_kifmm {

namespace {

constexpr std::array<blitzar_core::Scalar, KifmmWorkspace::SurfaceOrder> SurfaceCoordinates{
    -1.0, -0.5, 0.5, 1.0};

[[nodiscard]] constexpr std::size_t SurfaceMatrixSize() noexcept
{
    return KifmmWorkspace::SurfaceNodeCount * KifmmWorkspace::SurfaceNodeCount;
}

[[nodiscard]] bool IsBoundary(std::size_t index) noexcept
{
    return index == 0 || index + 1 == KifmmWorkspace::SurfaceOrder;
}

} // namespace

std::size_t KifmmWorkspace::CheckedProduct(std::size_t left, std::size_t right)
{
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::length_error("KIFMM workspace capacity overflow");
    }

    return left * right;
}

std::size_t KifmmWorkspace::CheckedDepthCount(std::size_t max_depth)
{
    if (max_depth == std::numeric_limits<std::size_t>::max()) {
        throw std::length_error("KIFMM depth capacity overflow");
    }

    return max_depth + 1;
}

KifmmWorkspace::KifmmWorkspace(
    std::size_t cell_capacity, std::size_t max_depth, std::size_t target_capacity)
    : cell_capacity_(cell_capacity), max_depth_(max_depth), target_capacity_(target_capacity),
      equivalent_nodes_(SurfaceNodeCount), check_nodes_(SurfaceNodeCount),
      operators_(CheckedProduct(CheckedDepthCount(max_depth), SurfaceMatrixSize())),
      cell_weights_(CheckedProduct(cell_capacity, SurfaceNodeCount)),
      traversal_stack_(cell_capacity), near_interactions_(cell_capacity),
      far_interactions_(cell_capacity), staging_(target_capacity)
{
    InitializeSurfaceNodes();
}

blitzar_status KifmmWorkspace::Prepare(
    std::size_t cell_capacity, std::size_t target_capacity) noexcept
{
    if (cell_capacity <= cell_capacity_ && target_capacity <= target_capacity_) {
        return BLITZAR_STATUS_OK;
    }

    try {
        KifmmWorkspace candidate(cell_capacity, max_depth_, target_capacity);

        *this = std::move(candidate);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

std::size_t KifmmWorkspace::CellCapacity() const noexcept
{
    return cell_capacity_;
}

std::size_t KifmmWorkspace::MaxDepth() const noexcept
{
    return max_depth_;
}

std::size_t KifmmWorkspace::TargetCapacity() const noexcept
{
    return target_capacity_;
}

std::span<const blitzar_core::Vector3> KifmmWorkspace::EquivalentNodes() const noexcept
{
    return equivalent_nodes_;
}

std::span<const blitzar_core::Vector3> KifmmWorkspace::CheckNodes() const noexcept
{
    return check_nodes_;
}

std::span<blitzar_core::Scalar> KifmmWorkspace::Operators(std::size_t depth) noexcept
{
    if (depth > max_depth_) {
        return {};
    }

    const std::size_t matrix_size = SurfaceNodeCount * SurfaceNodeCount;

    return std::span<blitzar_core::Scalar>(operators_).subspan(depth * matrix_size, matrix_size);
}

std::span<blitzar_core::Scalar> KifmmWorkspace::CellWeights(std::size_t cell) noexcept
{
    if (cell >= cell_capacity_) {
        return {};
    }

    return std::span<blitzar_core::Scalar>(cell_weights_)
        .subspan(cell * SurfaceNodeCount, SurfaceNodeCount);
}

std::span<const blitzar_core::Scalar> KifmmWorkspace::CellWeights(std::size_t cell) const noexcept
{
    if (cell >= cell_capacity_) {
        return {};
    }

    return std::span<const blitzar_core::Scalar>(cell_weights_)
        .subspan(cell * SurfaceNodeCount, SurfaceNodeCount);
}

std::span<std::size_t> KifmmWorkspace::TraversalStack() noexcept
{
    return traversal_stack_;
}

std::span<std::size_t> KifmmWorkspace::NearInteractions() noexcept
{
    return near_interactions_;
}

std::span<std::size_t> KifmmWorkspace::FarInteractions() noexcept
{
    return far_interactions_;
}

std::span<blitzar_core::Vector3> KifmmWorkspace::Staging() noexcept
{
    return staging_;
}

void KifmmWorkspace::InitializeSurfaceNodes() noexcept
{
    std::size_t node = 0;

    for (std::size_t x = 0; x < SurfaceOrder; ++x) {
        for (std::size_t y = 0; y < SurfaceOrder; ++y) {
            for (std::size_t z = 0; z < SurfaceOrder; ++z) {
                if (!IsBoundary(x) && !IsBoundary(y) && !IsBoundary(z)) {
                    continue;
                }

                const blitzar_core::Scalar coordinate_x = SurfaceCoordinates[x];
                const blitzar_core::Scalar coordinate_y = SurfaceCoordinates[y];
                const blitzar_core::Scalar coordinate_z = SurfaceCoordinates[z];

                equivalent_nodes_[node] = {
                    0.75 * coordinate_x, 0.75 * coordinate_y, 0.75 * coordinate_z};

                check_nodes_[node] = {
                    1.35 * coordinate_x, 1.35 * coordinate_y, 1.35 * coordinate_z};

                ++node;
            }
        }
    }
}

} // namespace blitzar_kifmm
