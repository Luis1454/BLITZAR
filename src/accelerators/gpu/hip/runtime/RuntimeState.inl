#include "accelerators/gpu/hip/memory/Buffers.hpp"

namespace blitzar_hip {

struct Context::Impl final {
    Buffers buffers;
    std::unique_ptr<blitzar_trees::Octree> tree;
    blitzar_barnes_hut::BarnesHutSettings tree_settings{};
    bool tree_ready{};
    Fault fault{Fault::None};

    [[nodiscard]] blitzar_status EnsureBuffers(
        std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept;
    [[nodiscard]] blitzar_status UploadFullState(
        blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] blitzar_status UploadTargetState(
        blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] blitzar_status UploadSourceState(
        blitzar_core::ParticleStateView particles, blitzar_solvers::ForceRange range) noexcept;
    [[nodiscard]] blitzar_status UploadForces(blitzar_core::ForceView forces) noexcept;
    [[nodiscard]] blitzar_status QueueForceDownloads(std::size_t particle_count) noexcept;
    [[nodiscard]] blitzar_status Finish(
        std::size_t particle_count, blitzar_core::ForceView forces) noexcept;
    [[nodiscard]] blitzar_status UploadDirectInputs(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_solvers::ForceRange range,
        bool source_alias_target) noexcept;
    [[nodiscard]] blitzar_status PrepareTree(blitzar_core::ParticleStateView particles,
        blitzar_barnes_hut::BarnesHutSettings settings) noexcept;
    [[nodiscard]] blitzar_status EnsureTree(
        blitzar_barnes_hut::BarnesHutSettings settings) noexcept;
    [[nodiscard]] blitzar_status UploadTree(std::span<const blitzar_trees::Octree::Cell> cells,
        std::span<const std::size_t> indices) noexcept;
    [[nodiscard]] blitzar_status ComputeDirect(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
        blitzar_solvers::ForceRange range) noexcept;
    [[nodiscard]] blitzar_status ComputeBarnesHut(const BarnesHutComputeRequest& request) noexcept;
};

} // namespace blitzar_hip
