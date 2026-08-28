#ifndef BLITZAR_SOLVERS_SOLVER_CONTRACT_HPP
#define BLITZAR_SOLVERS_SOLVER_CONTRACT_HPP

#include <cstddef>
#include <cstdint>

namespace blitzar_solvers {

enum class SolverKind : std::uint8_t {
    Direct = 0,
    BarnesHut = 1,
    Fmm = 2,
    Pm = 3,
    TreePm = 4,
    Kifmm = 5
};

enum class SpatialResourceKind : std::uint8_t {
    Invalid = 0,
    None = 1,
    Octree = 2,
    Grid = 3,
    OctreeAndGrid = 4
};

enum class SolverWorkspaceKind : std::uint8_t {
    Invalid = 0,
    None = 1,
    KernelIndependentMultipole = 2
};

struct SolverResourceContract final {
    SolverKind solver{};
    SpatialResourceKind spatial{SpatialResourceKind::Invalid};
    SolverWorkspaceKind workspace{SolverWorkspaceKind::Invalid};

    [[nodiscard]] static constexpr SolverResourceContract For(SolverKind kind) noexcept
    {
        if (kind == SolverKind::Direct) {

            return {kind, SpatialResourceKind::None, SolverWorkspaceKind::None};
        }

        if (kind == SolverKind::BarnesHut || kind == SolverKind::Fmm) {

            return {kind, SpatialResourceKind::Octree, SolverWorkspaceKind::None};
        }

        if (kind == SolverKind::Kifmm) {

            return {
                kind, SpatialResourceKind::Octree, SolverWorkspaceKind::KernelIndependentMultipole};
        }

        if (kind == SolverKind::Pm) {

            return {kind, SpatialResourceKind::Grid, SolverWorkspaceKind::None};
        }

        if (kind == SolverKind::TreePm) {

            return {kind, SpatialResourceKind::OctreeAndGrid, SolverWorkspaceKind::None};
        }

        return {kind, SpatialResourceKind::Invalid, SolverWorkspaceKind::Invalid};
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        const SolverResourceContract expected = For(solver);

        return expected.spatial != SpatialResourceKind::Invalid && expected.spatial == spatial &&
               expected.workspace == workspace;
    }

    [[nodiscard]] constexpr bool RequiresOctree() const noexcept
    {
        return spatial == SpatialResourceKind::Octree ||
               spatial == SpatialResourceKind::OctreeAndGrid;
    }

    [[nodiscard]] constexpr bool RequiresGrid() const noexcept
    {
        return spatial == SpatialResourceKind::Grid ||
               spatial == SpatialResourceKind::OctreeAndGrid;
    }

    [[nodiscard]] constexpr bool RequiresPrivateWorkspace() const noexcept
    {
        return workspace == SolverWorkspaceKind::KernelIndependentMultipole;
    }
};

struct ForceRange final {
    std::size_t source_begin{};
    std::size_t source_end{};
    bool accumulate{};

    [[nodiscard]] bool IsValid(std::size_t source_count) const noexcept
    {
        return source_begin <= source_end && source_end <= source_count;
    }
};

} // namespace blitzar_solvers

#endif
