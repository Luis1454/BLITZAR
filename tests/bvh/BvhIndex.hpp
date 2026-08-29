#ifndef BLITZAR_TESTS_BVH_BVH_INDEX_HPP
#define BLITZAR_TESTS_BVH_BVH_INDEX_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace blitzar_bvh {

struct BvhNode final {
    static constexpr std::size_t InvalidIndex = std::numeric_limits<std::size_t>::max();

    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};
    std::size_t left{InvalidIndex};
    std::size_t right{InvalidIndex};
    std::size_t begin{};
    std::size_t count{};

    [[nodiscard]] bool IsLeaf() const noexcept;
};

struct BvhWorkspace final {
    std::vector<std::size_t> stack{};
    std::vector<std::size_t> candidates{};

    [[nodiscard]] std::size_t MemoryBytes() const noexcept;
};

class BvhIndex final {
public:
    explicit BvhIndex(std::size_t leaf_size);

    [[nodiscard]] bool Build(const blitzar_neighborhood::NeighborFrame& frame);
    [[nodiscard]] bool Refit(const blitzar_neighborhood::NeighborFrame& frame);
    [[nodiscard]] blitzar_neighborhood::NeighborSet Query(
        const blitzar_neighborhood::NeighborFrame& frame, blitzar_core::Scalar radius,
        BvhWorkspace& workspace) const;
    [[nodiscard]] std::size_t MemoryBytes() const noexcept;
    [[nodiscard]] std::size_t ParticleCount() const noexcept;
    [[nodiscard]] std::size_t LeafSize() const noexcept;
    [[nodiscard]] std::uint64_t Hash() const noexcept;

private:
    struct BuildTask final {
        std::size_t begin{};
        std::size_t end{};
        std::size_t node{};
    };

    [[nodiscard]] static bool IsValidFrame(
        const blitzar_neighborhood::NeighborFrame& frame) noexcept;
    [[nodiscard]] BvhNode ComputeBounds(const blitzar_neighborhood::NeighborFrame& frame,
        std::size_t begin, std::size_t end) const noexcept;
    [[nodiscard]] static blitzar_core::Scalar Coordinate(
        blitzar_core::Vector3 position, int axis) noexcept;
    [[nodiscard]] static int LongestAxis(const BvhNode& node) noexcept;
    [[nodiscard]] static bool Intersects(const BvhNode& node, blitzar_core::Vector3 point,
        blitzar_core::Scalar radius_squared) noexcept;
    [[nodiscard]] static blitzar_core::Scalar DistanceToAxis(blitzar_core::Scalar value,
        blitzar_core::Scalar minimum, blitzar_core::Scalar maximum) noexcept;

    std::size_t leaf_size_{};
    std::size_t particle_count_{};
    bool built_{false};
    std::vector<BvhNode> nodes_{};
    std::vector<std::size_t> indices_{};
    std::vector<BuildTask> tasks_{};
};

} // namespace blitzar_bvh

#endif
