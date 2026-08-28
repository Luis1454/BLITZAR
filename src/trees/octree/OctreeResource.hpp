#ifndef BLITZAR_TREES_OCTREE_OCTREE_RESOURCE_HPP
#define BLITZAR_TREES_OCTREE_OCTREE_RESOURCE_HPP

#include "core/CoreTypes.hpp"
#include "trees/octree/Octree.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>

namespace blitzar_trees {

struct OctreeResourceConfig final {
    std::size_t max_particles{};
    std::size_t max_cells{};
    std::size_t leaf_capacity{};
    std::size_t max_depth{};

    [[nodiscard]] bool IsValid() const noexcept;
};

class OctreeResource final {
public:
    explicit OctreeResource(OctreeResourceConfig config);

    OctreeResource(const OctreeResource&) = delete;
    OctreeResource& operator=(const OctreeResource&) = delete;
    OctreeResource(OctreeResource&&) noexcept = default;
    OctreeResource& operator=(OctreeResource&&) noexcept = default;

    [[nodiscard]] blitzar_status Prepare(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] OctreeView View() const noexcept;
    [[nodiscard]] bool IsCurrent(OctreeView view) const noexcept;
    [[nodiscard]] std::size_t MaxParticles() const noexcept;
    [[nodiscard]] std::size_t MaxCells() const noexcept;
    [[nodiscard]] std::size_t BuildCount() const noexcept;
    [[nodiscard]] std::size_t RefitCount() const noexcept;

private:
    OctreeResourceConfig config_;
    Octree tree_;
};

} // namespace blitzar_trees

#endif
