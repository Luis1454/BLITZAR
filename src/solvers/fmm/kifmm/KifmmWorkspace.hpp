#ifndef BLITZAR_SOLVERS_FMM_KIFMM_KIFMM_WORKSPACE_HPP
#define BLITZAR_SOLVERS_FMM_KIFMM_KIFMM_WORKSPACE_HPP

#include "core/CoreTypes.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <span>
#include <vector>

namespace blitzar_kifmm {

class KifmmWorkspace final {
public:
    static constexpr std::size_t SurfaceOrder = 4;
    static constexpr std::size_t SurfaceNodeCount = 56;

    KifmmWorkspace(std::size_t cell_capacity, std::size_t max_depth, std::size_t target_capacity);

    KifmmWorkspace(const KifmmWorkspace&) = delete;
    KifmmWorkspace& operator=(const KifmmWorkspace&) = delete;
    KifmmWorkspace(KifmmWorkspace&&) noexcept = default;
    KifmmWorkspace& operator=(KifmmWorkspace&&) noexcept = default;

    [[nodiscard]] blitzar_status Prepare(
        std::size_t cell_capacity, std::size_t target_capacity) noexcept;
    [[nodiscard]] std::size_t CellCapacity() const noexcept;
    [[nodiscard]] std::size_t MaxDepth() const noexcept;
    [[nodiscard]] std::size_t TargetCapacity() const noexcept;
    [[nodiscard]] std::span<const blitzar_core::Vector3> EquivalentNodes() const noexcept;
    [[nodiscard]] std::span<const blitzar_core::Vector3> CheckNodes() const noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> Operators(std::size_t depth) noexcept;
    [[nodiscard]] std::span<blitzar_core::Scalar> CellWeights(std::size_t cell) noexcept;
    [[nodiscard]] std::span<const blitzar_core::Scalar> CellWeights(
        std::size_t cell) const noexcept;
    [[nodiscard]] std::span<std::size_t> TraversalStack() noexcept;
    [[nodiscard]] std::span<std::size_t> NearInteractions() noexcept;
    [[nodiscard]] std::span<std::size_t> FarInteractions() noexcept;
    [[nodiscard]] std::span<blitzar_core::Vector3> Staging() noexcept;

private:
    [[nodiscard]] static std::size_t CheckedProduct(std::size_t left, std::size_t right);
    [[nodiscard]] static std::size_t CheckedDepthCount(std::size_t max_depth);
    void InitializeSurfaceNodes() noexcept;

    std::size_t cell_capacity_{};
    std::size_t max_depth_{};
    std::size_t target_capacity_{};
    std::vector<blitzar_core::Vector3> equivalent_nodes_;
    std::vector<blitzar_core::Vector3> check_nodes_;
    std::vector<blitzar_core::Scalar> operators_;
    std::vector<blitzar_core::Scalar> cell_weights_;
    std::vector<std::size_t> traversal_stack_;
    std::vector<std::size_t> near_interactions_;
    std::vector<std::size_t> far_interactions_;
    std::vector<blitzar_core::Vector3> staging_;
};

} // namespace blitzar_kifmm

#endif
